
#include "App.h"
#include "EventLoop.h"
#include "MonitorProc.h"
#include "log.h"

int main()
{
    init_spdlog("zhengze");
    SPDLOG_INFO("Build Time: {} {}.", __DATE__, __TIME__);
    SPDLOG_INFO("Git commit id: {}.", GIT_COMMIT_ID);
    SPDLOG_INFO("Git branch name: {}.", GIT_BRANCH_NAME);

    g_APP;

    g_APP.signal(SIGCHLD, [](const int sig){
        g_MonitorProc.onChildStatusChange(sig);
        return false;
    });
    g_APP.signal(KILL_CHILD_PROC_SIGNAL, [](const int sig) { return true; }); // 对该信号不做处理
    g_APP.coreSignal([](const int sig) {
        g_MonitorProc.stop();
        deinit_spdlog();
        return true;
    });

    g_EventLoop.start();

    g_APP.exec();

    SPDLOG_WARN("The APP has begun to exit.");

    g_EventLoop.stop();
    deinit_spdlog();
    return 0;
}
