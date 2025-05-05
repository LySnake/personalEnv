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
    static bool onChildStatusChange(const int sig);

    // 版本一致性检查时，
    void start();
    void stop();

  private:
    // 处理退出的子进程
    // child_pid:子进程pid。
    void onTermChildProcess(const pid_t child_pid);

    // 拉起对应的子进程
    // proc_name:需要拉起的程序
    void launchProc(const std::string &app_name);
    // 终止对应的子进程
    // proc_name:需要终止的程序
    void termProc(const std::string &app_name);

  private:
    MonitorProc();

  private:
    std::atomic_bool is_exit_; // 是否退出监听
    //      <pid,   app_name>
    std::map<pid_t, std::string> child_process_; // 监控的子进程
};

#endif // MONITOR_PROC_H