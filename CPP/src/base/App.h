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

    /****************************************************
     * 函数说明：设置默认信号屏蔽集，让信号交给sigwait处理。
     *          NOTE:所有ign或term信号集合
     *****************************************************/
    static void setDefSignalSet();

    /****************************************************
     * 函数说明：对当前进程的信号屏蔽集，添加信号
     *****************************************************/
    static void addSignalSet(std::initializer_list<int> &sig_list);

    /****************************************************
     * 函数说明：对当前进程的信号屏蔽集，删除信号
     *****************************************************/
    static void delSignalSet(std::initializer_list<int> &sig_list);

    /****************************************************
     * 函数说明：清空信号屏蔽集，子线程和子进程都会继承，所以必要时需要清理。
     *
     *          Note:fork的子进程会继承信号屏蔽集，exec并不会更改。
     *          Note:system()和popen()都会继承信号屏蔽集。
     *****************************************************/
    static void emptySignalSet();

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
     * 函数说明：在程序初始化时，主线程提前准备好。在捕获core信号时，执行一段程序
     *         NOTE:返回值不做处理
     *
     * handler: core信号处理程序
     *****************************************************/
    static void coreSignal(SignalHandler handler);

    /****************************************************
     * 函数说明：阻塞当前线程，调用sigwait等待信号处理。
     *          NOTE:sigwait处理调用时，当前进程信号屏蔽集
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