#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <string>
#include <vector>

#include "EventLoop.h"
#include "MonitorProc.h"
#include "log.h"
#include "os.h"



namespace
{
    // 启动程序的配置信息
    struct AppConf
    {
        const std::string app_name;                  // 程序名(可运行的名字，必要时带路径)
        const std::vector<std::string> args;         // 程序启动时的启动参数
        const std::vector<std::string> restart_apps; // 需要重启的关联程序(包括自己，有启动先后顺序 ->)
    };

    const std::array<AppConf, 3> apps{};
}; // namespace

MonitorProc &MonitorProc::get_instance()
{
    static MonitorProc ins;
    return ins;
}

MonitorProc::MonitorProc() : is_exit_{true} {}

bool MonitorProc::onChildStatusChange(const int sig)
{
    int wstatus = -1;

    // 怕信号丢失
    // while (true)
    {
        const pid_t pid = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid == -1)
        {
            LOG_ERROR("System call failed. [%s].", os::POSIX_errno().c_str());
            return false;
        }
        // 无变化，立即返回
        else if (pid == 0)
        {
            LOG_WARN("Failed to monitor the status change of the child process.");
            // break;
        }
        else
        {
            bool need_launch = true;
            LOG_INFO("The exited child process(pid:%d). wstatus:%d", pid, wstatus);
            if (WIFEXITED(wstatus))
            {
                const int exit_code = WEXITSTATUS(wstatus);
                LOG_WARN("The child process[pid:%d] exited normally with exit code %d.", pid, exit_code);
            }
            else if (WIFSIGNALED(wstatus))
            {
                const int sig = WTERMSIG(wstatus);
                LOG_WARN("The child process[pid:%d] was terminated by a signal. The signal was %d.", pid, sig);
                if (KILL_CHILD_PROC_SIGNAL == sig)
                {
                    need_launch = false;
                }
            }
            else if (WIFSTOPPED(wstatus))
            {
                // 接收到信号，导致子进程STOP(暂停运行)
                LOG_WARN("The child process[pid:%d] was suspended due to receiving a signal.", pid);
            }
            else if (WIFCONTINUED(wstatus))
            {
                // 接收到信号，让子进程CONTINUE(继续运行)
                LOG_WARN("The child process[pid:%d] continues after receiving the signal.", pid);
            }

            if (need_launch)
            {
                g_MasterScheduler.addAsynTask([child_pid = pid]() { g_MonitorProc.onExitChildProc(child_pid); });
            }
        }
    }

    return true;
}

void MonitorProc::start()
{
    is_exit_ = false;
    for (auto &app : apps)
    {
        launchProc(app.app_name);
    }
}

void MonitorProc::stop()
{
    // const pid_t gpid = getpgrp();
    // 向进程组发送信号。
    // 0:标识的进程组与调用方相同。
    is_exit_ = true;
    const int ret = kill(0, KILL_CHILD_PROC_SIGNAL);
    if (ret == -1)
    {
        LOG_ERROR("System call failed. [%d]. %s.", ret, os::POSIX_errno().c_str());
    }

    child_proc_.clear();
}

void MonitorProc::onExitChildProc(const pid_t child_pid)
{
    auto iter = child_proc_.find(child_pid);
    if (iter == child_proc_.end())
    {
        LOG_WARN("Unidentified child process. pid is %d.", child_pid);
    }
    else
    {
        const std::string term_app_name = std::move(iter->second);
        child_proc_.erase(iter);
        LOG_WARN("Terminated child process, pid:%d, name:%s.", child_pid, term_app_name.c_str());
        if (!is_exit_)
        {
            for (auto &app : apps)
            {
                if (app.app_name == term_app_name)
                {
                    for (auto &app_name : app.restart_apps)
                    {
                        termProc(app_name);
                    }
                    for (auto &app_name : app.restart_apps)
                    {
                        launchProc(app_name);
                    }
                    return;
                }
            }
        }
    }
}

void MonitorProc::launchProc(const std::string &app_name)
{
    for (auto &app : apps)
    {
        if (app.app_name == app_name)
        {
            const pid_t pid = fork();
            if (pid == -1)
            {
                LOG_ERROR("System call failed. %s.", os::POSIX_errno().c_str());
            }
            else if (pid == 0)
            {
                // 子进程
                std::string cli;
                cli.reserve(256);
                std::vector<char *> argv;
                argv.reserve(app.args.size() + 1);

                cli += app.app_name + " ";
                for (auto &arg : app.args)
                {
                    argv.push_back(const_cast<char *>(arg.c_str()));
                    cli += arg + " ";
                }
                argv.push_back(nullptr);

                LOG_INFO("launch app:%s.", cli.c_str());
                const int ret = execv(app.app_name.c_str(), argv.data());
                UNUSED(ret);
                // 成功时，该行不执行，已经到子进程代码区。
                LOG_ERROR("System call failed. %s.", os::POSIX_errno().c_str());
            }
            else
            {
                // 父进程
                child_proc_.emplace(pid, app_name);
            }

            break;
        }
    }
}

void MonitorProc::termProc(const std::string &app_name)
{
    for (auto &iter : child_proc_)
    {
        if (iter.second == app_name)
        {
            const auto pid = iter.first;
            child_proc_.erase(pid);

            const int ret = kill(pid, KILL_CHILD_PROC_SIGNAL);
            if (ret == -1 && errno != ESRCH) // 可能进程已经不存在了。
            {
                LOG_ERROR("System call failed. [%d]. %s.", ret, os::POSIX_errno().c_str());
            }
            break;
        }
    }
}