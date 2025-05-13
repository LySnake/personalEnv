#include <string.h>

#include "App.h"
#include "log.h"
#include "os.h"

namespace
{
    App::SignalHandler s_core_signal_handler;
    // 获取:所有默认是ign行为的信号集
    sigset_t fillIgnSigs()
    {
        sigset_t set;
        sigemptyset(&set);

        sigaddset(&set, SIGCHLD);
        sigaddset(&set, SIGCLD);
        sigaddset(&set, SIGURG);
        sigaddset(&set, SIGWINCH);

        return set;
    }

    // 获取:所有默认是core行为的信号集
    // sigset_t fillCoreSigs()
    // {
    //     sigset_t set;
    //     sigemptyset(&set);

    //     sigaddset(&set, SIGABRT);
    //     sigaddset(&set, SIGBUS);
    //     sigaddset(&set, SIGFPE);
    //     sigaddset(&set, SIGILL);
    //     sigaddset(&set, SIGIOT);
    //     sigaddset(&set, SIGQUIT);
    //     sigaddset(&set, SIGSEGV);
    //     sigaddset(&set, SIGSYS);
    //     sigaddset(&set, SIGTRAP);
    //     sigaddset(&set, SIGXCPU);
    //     sigaddset(&set, SIGXFSZ);

    //     return set;
    // }

    // 获取:所有默认是term行为的信号集
    sigset_t fillTermSigs()
    {
        sigset_t set;
        sigemptyset(&set);

        sigaddset(&set, SIGALRM);
        sigaddset(&set, SIGHUP);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGIO);
        // sigaddset(&set, SIGKILL);   // 无法捕获
        sigaddset(&set, SIGPIPE);
        sigaddset(&set, SIGPOLL);
        sigaddset(&set, SIGPROF);
        sigaddset(&set, SIGPWR);
        sigaddset(&set, SIGSTKFLT);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGUSR1);
        sigaddset(&set, SIGUSR2);
        sigaddset(&set, SIGVTALRM);

        return set;
    }

    void core_dump(int sig)
    {
        signal(sig, SIG_DFL);
        SPDLOG_ERROR("Signal(Core) ({}:{}) received.", sig, strsignal(sig));
        os::printBacktrace("Signal(Core).");

        if (s_core_signal_handler)
        {
            s_core_signal_handler(sig);
        }

        raise(sig);
    }
} // namespace

App &App::get_instance()
{
    static App ins;
    return ins;
}

App::App() : sig_callbacks_{}, exit_code_{EXIT_SUCCESS}
{
    // 生成CORE文件的signal
    struct sigaction sa;
    sa.sa_handler = core_dump;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGIOT, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGSYS, &sa, nullptr);
    sigaction(SIGXCPU, &sa, nullptr);
    sigaction(SIGXFSZ, &sa, nullptr);
}

void App::addSignalSet(std::initializer_list<int> &sig_list)
{
    sigset_t add_set;
    sigemptyset(&add_set);

    for (const auto sig : sig_list)
    {
        sigaddset(&add_set, sig);
    }

    int ret = pthread_sigmask(SIG_BLOCK, &add_set, nullptr);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }
}

void App::delSignalSet(std::initializer_list<int> &sig_list)
{
    sigset_t del_set;
    sigemptyset(&del_set);

    for (const auto sig : sig_list)
    {
        sigaddset(&del_set, sig);
    }

    int ret = pthread_sigmask(SIG_UNBLOCK, &del_set, nullptr);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }
}

void App::setDefSignalSet()
{
    const auto ign_set = fillIgnSigs();
    int ret = pthread_sigmask(SIG_BLOCK, &ign_set, nullptr);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }

    const auto term_set = fillTermSigs();
    ret = pthread_sigmask(SIG_BLOCK, &term_set, nullptr);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }
}

void App::emptySignalSet()
{
    sigset_t empty_set;
    sigemptyset(&empty_set);

    auto ret = sigprocmask(SIG_SETMASK, &empty_set, NULL);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }
}

void App::signal(const int sig, SignalHandler handler) { sig_callbacks_[sig] = std::move(handler); }

void App::coreSignal(SignalHandler handler) { s_core_signal_handler = std::move(handler); }

int App::exec()
{
    sigset_t mask_set;
    int ret = pthread_sigmask(SIG_SETMASK, nullptr, &mask_set);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }

    int sig = 0;
    while (true)
    {
        const int ret = sigwait(&mask_set, &sig);
        if (ret == 0)
        {
            SPDLOG_WARN("Capture the signal({}:{}).", sig, strsignal(sig));
            auto &cb = sig_callbacks_[sig];

            bool is_replace_action = false;
            if (cb)
            {
                is_replace_action = cb(sig);
            }

            const auto term_set = fillTermSigs();
            if (!is_replace_action && sigismember(&term_set, sig))
            {
                break;
            }
        }
        else
        {
            SPDLOG_ERROR("System call failed. [{}].", ret);
            break;
        }
    }

    return exit_code_;
}

void App::exit(const int exit_code)
{
    exit_code_ = exit_code;
    int ret = raise(KILL_SELF_PROC_SIGNAL);
    if (ret != 0)
    {
        SPDLOG_ERROR("System call failed. [{}].", ret);
        ::exit(1);
    }
}