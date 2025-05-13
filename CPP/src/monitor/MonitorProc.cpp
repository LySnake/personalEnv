#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <string>
#include <vector>

#include "App.h"
#include "Config.h"
#include "EventLoop.h"
#include "MonitorProc.h"
#include "log.h"
#include "os.h"


using namespace std::chrono_literals;

namespace
{
    using SteadyTimePoint = std::chrono::steady_clock::time_point; // 单调时钟使用的time_point类型

    // 启动程序的配置信息
    struct AppConf
    {
        const std::string app_name;          // 程序名(可运行的名字，必要时带路径)
        const std::vector<std::string> args; // 程序启动时的启动参数
        const std::vector<std::string> restart_dependent_apps; // 需要重启的关联程序(包括自己，有启动先后顺序 左->右)
        uint32_t restart_total_count;                          // 进程总重启次数
        SteadyTimePoint start_time_point;                      // 进程启动时间
    };

    std::array<AppConf, 1> apps{}; 
};     // namespace

MonitorProc &MonitorProc::get_instance()
{
    static MonitorProc ins;
    return ins;
}

MonitorProc::MonitorProc() : is_exit_{true} {}

bool MonitorProc::onChildStatusChange(const int sig)
{
    int wstatus = -1;

    std::vector<pid_t> child_pids;

    for (const auto &iter : child_process_)
    {
        child_pids.push_back(iter.first);
    }

    for (const auto child_pid : child_pids)
    {
        const pid_t pid = waitpid(child_pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid == -1)
        {
            SPDLOG_ERROR("System call failed. [{}].", os::POSIX_errno());
            return false;
        }
        // 无变化，立即返回
        else if (pid == 0)
        {
            // SPDLOG_WARN("Failed to monitor the status change of the child process.");
        }
        else
        {
            bool need_launch = true;
            SPDLOG_INFO("The exited child process(pid:{}). wstatus:{}", pid, wstatus);
            if (WIFEXITED(wstatus))
            {
                const int exit_code = WEXITSTATUS(wstatus);
                SPDLOG_WARN("The child process[pid:{}] exited normally with exit code {}.", pid, exit_code);
            }
            else if (WIFSIGNALED(wstatus))
            {
                const int sig = WTERMSIG(wstatus);
                SPDLOG_WARN("The child process[pid:{}] was terminated by a signal. The signal was {}.", pid, sig);
                if (KILL_CHILD_PROC_SIGNAL == sig)
                {
                    need_launch = false;
                }
            }
            else if (WIFSTOPPED(wstatus))
            {
                // 接收到信号，导致子进程STOP(暂停运行)
                SPDLOG_WARN("The child process[pid:{}] was suspended due to receiving a signal.", pid);
                need_launch = false;
            }
            else if (WIFCONTINUED(wstatus))
            {
                // 接收到信号，让子进程CONTINUE(继续运行)
                SPDLOG_WARN("The child process[pid:{}] continues after receiving the signal.", pid);
                need_launch = false;
            }

            if (need_launch)
            {
                g_MonitorProc.onTermChildProcess(pid);
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
    SPDLOG_INFO("Stop all child process.");
    // const pid_t gpid = getpgrp();
    // 向进程组发送信号。
    // 0:标识的进程组与调用方相同。
    is_exit_ = true;
    const int ret = kill(0, KILL_CHILD_PROC_SIGNAL);
    if (ret == -1)
    {
        SPDLOG_ERROR("System call failed. [{}]: {}.", ret, os::POSIX_errno());
    }

    child_process_.clear();
}

void MonitorProc::onTermChildProcess(const pid_t child_pid)
{
    auto iter = child_process_.find(child_pid);
    if (iter == child_process_.end())
    {
        SPDLOG_WARN("Unidentified child process. pid is {}.", child_pid);
    }
    else
    {
        const std::string term_app_name = std::move(iter->second);
        child_process_.erase(iter);
        SPDLOG_WARN("Terminated child process, pid:{}, name:{}.", child_pid, term_app_name);
        if (!is_exit_)
        {
            for (auto &app : apps)
            {
                if (app.app_name == term_app_name)
                {
                    for (const auto &app_name : app.restart_dependent_apps)
                    {
                        termProc(app_name);
                    }
                    for (const auto &app_name : app.restart_dependent_apps)
                    {
                        launchProc(app_name);
                    }
                    break;
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
                SPDLOG_ERROR("System call failed. {}.", os::POSIX_errno());
            }
            else if (pid == 0)
            {
                // 子进程
                g_APP.emptySignalSet();

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

                // fork的子进程中，不应该通过spdlog打印。原因是spdlog如果有锁，可能会有问题。仅仅调试的话，可以。
                // SPDLOG_INFO("launch child process:{}.", cli);
                printf("launch child process:%s.", cli.c_str());
                const int ret = execv(app.app_name.c_str(), argv.data());
                UNUSED(ret);
                // 成功时，该行不执行，已经到子进程代码区。
                // SPDLOG_ERROR("System call failed. {}.", os::POSIX_errno());
                printf("System call failed. %s.", os::POSIX_errno().c_str());
                g_APP.exit(EXIT_FAILURE);
            }
            else
            {
                // 父进程
                child_process_.insert({pid, app_name});
                app.restart_total_count++;
                const auto now = std::chrono::steady_clock().now();

                if (app.start_time_point == SteadyTimePoint())
                {
                    SPDLOG_WARN("launch child process. pid is {}. name is {}.", pid, app_name);
                }
                else
                {
                    const auto elapsed = now - app.start_time_point;
                    SPDLOG_WARN("launch child process.pid is {}. name is {}. launch count:{}. keep-alive time:{}s. ",
                                pid, app_name, app.restart_total_count,
                                std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
                }

                app.start_time_point = now;
            }

            break;
        }
    }
}

void MonitorProc::termProc(const std::string &app_name)
{
    for (auto &iter : child_process_)
    {
        if (iter.second == app_name)
        {
            const auto pid = iter.first;
            child_process_.erase(pid);

            SPDLOG_WARN("Terminated child process. pid is {}. name is {}.", pid, app_name);
            const int ret = kill(pid, KILL_CHILD_PROC_SIGNAL);
            if (ret == -1 && errno != ESRCH) // 可能进程已经不存在了。
            {
                SPDLOG_ERROR("System call failed. [{}]. {}.", ret, os::POSIX_errno());
            }
            break;
        }
    }
}