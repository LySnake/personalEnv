#ifndef APP_H
#define APP_H

#include <signal.h>

#include <atomic>
#include <functional>
#include <map>

#include "Define.h"

#define g_APP App::get_instance()

constexpr int KILL_SELF_PROC_SIGNAL = SIGUSR2; // 唤醒main线程，退出进程的信号

// APP进程环境控制类
class App
{
  public:
    static App &get_instance();

    // true: 替换信号的默认行为  false: 不替换,执行玩callback后继续sigwait
    // 默认行为有 ign、core、term、(stop、continue:不捕获、不处理)
    // 但是此处不处理core相关信号

    /****************************************************
     * 函数说明：默认行为有 ign、core、term、(stop、continue:不捕获、不处理)
     *          但是此处不处理core相关信号
     *
     * sig: 触发的signal
     * return: true: 替换信号的默认行为  false: 不替换,执行玩callback后继续sigwait
     *****************************************************/
    using SignalHandler = std::function<bool(const int sig)>;

    /****************************************************
     * 函数说明：在程序初始化时，主线程提前准备好。哪些signal
     *          要被监听。
     *
     * sig: 监听且被需要处理的信号
     * handler: 信号处理函数
     *****************************************************/
    void signal(const int sig, SignalHandler handler);

    /****************************************************
     * 函数说明：阻塞当前线程，调用sigwait等待信号处理。
     *****************************************************/
    int exec();

    // 唤醒APP::exec()并终止

    /****************************************************
     * 函数说明：唤醒APP::exec()并终止
     *
     * exit_code:APP::exec()返回值
     *****************************************************/
    void exit(const int exit_code);

  private:
    App();

  private:
    std::map<int, SignalHandler> sig_callbacks_; // 注册的信号处理函数
    std::atomic_int exit_code_;                  // 使用App::exit()时的退出码
};

#endif // APP_H