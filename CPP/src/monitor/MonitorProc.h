#ifndef MONITOR_PROC_H
#define MONITOR_PROC_H

#include <signal.h>
#include <sys/types.h>

#include <atomic>
#include <map>
#include <string>

#include "Define.h"

#define g_MonitorProc MonitorProc::get_instance()

constexpr int KILL_CHILD_PROC_SIGNAL = SIGUSR1; // 杀子进程，使用的信号

class MonitorProc
{
  public:
    static MonitorProc &get_instance();

    DISALLOW_COPY_AND_MOVE_OPERATOR(MonitorProc);

    // 监听 SIGCHLD 信号的处理函数
    bool onChildStatusChange(const int sig);

    // 拉起所有预设的子程序
    void start();
    void stop();

    // 拉起对应的子进程
    // proc_name:需要拉起的程序
    void launchProc(const std::string &app_name);
    // 终止对应的子进程
    // proc_name:需要终止的程序
    void termProc(const std::string &app_name);

  private:
    // 处理退出的子进程
    // child_pid:子进程pid。
    void onTermChildProcess(const pid_t child_pid);

  private:
    MonitorProc();
    void init();

  private:
    using SteadyTimePoint = std::chrono::steady_clock::time_point; // 单调时钟使用的time_point类型

    // 启动程序的配置信息
    struct AppConf
    {
        AppConf(std::string &&app_name_, std::string &&core_nums_, std::vector<std::string> &&args_,
                std::vector<std::string> &&restart_dependent_apps_, const float post_fork_sleep_s_)
            : app_name{std::move(app_name_)}, core_nums{std::move(core_nums_)}, args{std::move(args_)},
              restart_dependent_apps{std::move(restart_dependent_apps_)}, post_fork_sleep_s{post_fork_sleep_s_},
              restart_total_count{0}, start_time_point{SteadyTimePoint()}
        {
        }
        DEFAULT_COPY_AND_MOVE_OPERATOR(AppConf);
        const std::string app_name;          // 程序名(可运行的名字，必要时带路径)
        const std::string core_nums;         // 进程绑定的核心
        const std::vector<std::string> args; // 程序启动时的启动参数
        const std::vector<std::string> restart_dependent_apps; // 需要重启的关联程序(包括自己，有启动先后顺序 左->右)
        float post_fork_sleep_s;                               // fork后，要sleep多少秒
        uint32_t restart_total_count;                          // 进程总重启次数
        SteadyTimePoint start_time_point;                      // 进程启动时间
    };

  private:
    std::vector<AppConf> apps_;
    std::atomic_bool is_exit_; // 是否退出监听
    //      <pid,   app_name>
    std::map<pid_t, std::string> child_process_; // 监控的子进程
};

#endif // MONITOR_PROC_H