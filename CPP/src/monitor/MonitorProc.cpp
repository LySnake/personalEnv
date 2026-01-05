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
#include "File.h"
#include "MonitorProc.h"
#include "log.h"
#include "nlohmann/json.hpp"
#include "os.h"

#define JSON_CONFIG_FILE ALIEN_CELL_CONFIG_DIR "/config.json"

#define JSON_KEY_APPS "apps"
#define JSON_KEY_APP_NAME "app_name"
#define JSON_KEY_ARGS "args"
#define JSON_KEY_DEPENDENT_APPS "restart_dependent_apps"
#define JSON_KEY_POST_FORK_SLEEP "post_fork_sleep_s"
#define JSON_KEY_PROC_CORE_NUMS "proc_core_nums"

using namespace std::chrono_literals;

namespace
{
    // 把进程pid绑定到具体的CPU核心上
    //            [核心编号范围]  [pid]
    // taskset -pc 2-3 475
    void bind_cpu_core(const pid_t pid, const std::string core_nums)
    {
        const std::string shell_cmd_bind_cpu = "taskset -pc " + core_nums + " " + std::to_string(pid);
        auto result = os::exec_shell(shell_cmd_bind_cpu);
        if (!result.first)
        {
            SPDLOG_ERROR("Failed to bind the process to the CPU core. pid is {}. stderr/stdout is {}.", pid,
                         result.second);
        }
    }
} // namespace

MonitorProc &MonitorProc::get_instance()
{
    static MonitorProc ins;
    return ins;
}

MonitorProc::MonitorProc() : is_exit_{true} { init(); }

void MonitorProc::init()
{
    {
        const pid_t current_pid = getpid();
        bind_cpu_core(current_pid, "0-1");
    }

    File config_file(JSON_CONFIG_FILE);
    std::string file_content;
    if (config_file.open(true, false) && config_file.readAll(file_content))
    {
        SPDLOG_INFO("File: {}, content :{}", JSON_CONFIG_FILE, file_content);
        try
        {
            const nlohmann::json config = nlohmann::json::parse(file_content);
            if (config[JSON_KEY_APPS].is_array())
            {
                auto apps = config[JSON_KEY_APPS];
                for (const auto &item : apps)
                {
                    if (item[JSON_KEY_APP_NAME].is_string() && item[JSON_KEY_ARGS].is_array() &&
                        item[JSON_KEY_DEPENDENT_APPS].is_array())
                    {
                        auto app_name = item[JSON_KEY_APP_NAME].get<std::string>();
                        const auto json_args = item[JSON_KEY_ARGS];
                        const auto json_restart_dependent_apps = item[JSON_KEY_DEPENDENT_APPS];
                        const float post_fork_sleep_s = item.value(JSON_KEY_POST_FORK_SLEEP, 0.0f);
                        auto core_nums = item[JSON_KEY_PROC_CORE_NUMS].get<std::string>();

                        std::vector<std::string> args;
                        std::vector<std::string> restart_dependent_apps;

                        for (const auto &arg : json_args)
                        {
                            if (arg.is_string())
                            {
                                args.emplace_back(arg.get<std::string>());
                            }
                            else
                            {
                                SPDLOG_ERROR(
                                  "Failed to parse JSON. Error: {} field has an incorrect type. app_name is {}.",
                                  JSON_KEY_ARGS, app_name);
                            }
                        }

                        for (const auto &app : json_restart_dependent_apps)
                        {
                            if (app.is_string())
                            {
                                restart_dependent_apps.emplace_back(app.get<std::string>());
                            }
                            else
                            {
                                SPDLOG_ERROR(
                                  "Failed to parse JSON. Error: {} field has an incorrect type. app_name is {}.",
                                  JSON_KEY_DEPENDENT_APPS, app_name);
                            }
                        }

                        apps_.emplace_back(AppConf{std::move(app_name), std::move(core_nums), std::move(args),
                                                   std::move(restart_dependent_apps), post_fork_sleep_s});
                    }
                    else
                    {
                        SPDLOG_ERROR("Failed to parse the JSON. Field type error.");
                    }
                }
            }
            else
            {
                SPDLOG_ERROR("key({}) is not array. ", JSON_KEY_APPS);
            }
        }
        catch (const std::exception &e)
        {
            SPDLOG_ERROR("Exception occurred while parsing the JSON file. file is {}.  exception msg is {}.",
                         JSON_CONFIG_FILE, e.what());
        }
    }
    else
    {
        SPDLOG_ERROR(
          "Failed to operate the configuration file. Please check the configuration file({" JSON_CONFIG_FILE "}).");
    }
}

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
    for (const auto &app : apps_)
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

        { // 进程终止时，收集dmesg
            for (auto &app : apps_)
            {
                if (app.app_name == term_app_name)
                {
                    std::string shell_cmd_dmesg = "dmesg > " ALIEN_CELL_LOG_RW_DIR;
                    if (app.args.empty())
                    {
                        shell_cmd_dmesg += "Term";
                    }
                    else
                    {
                        shell_cmd_dmesg += app.args[0];
                    }
                    shell_cmd_dmesg += "_dmesg.log";

                    os::exec_shell(shell_cmd_dmesg);
                }
            }
        }

        if (!is_exit_)
        {
            for (auto &app : apps_)
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
    for (auto &app : apps_)
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
                bind_cpu_core(pid, app.core_nums);
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

                if (app.post_fork_sleep_s > 0.01)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(
                      static_cast<std::chrono::milliseconds::rep>(app.post_fork_sleep_s * 1000)));
                }
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

            // 晚点回收
            g_MasterScheduler.addTimerTask(
              [pid]() {
                  int wstatus = 0;
                  waitpid(pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
                  return false;
              },
              3s);
            break;
        }
    }
}