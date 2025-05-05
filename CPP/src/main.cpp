
#include "App.h"
#include "EventLoop.h"
#include "MonitorProc.h"
#include "log.h"

int main()
{
    LOG_INFO("Build Time: %s %s.", __DATE__, __TIME__);
    LOG_INFO("Git commit id: %s.", GIT_COMMIT_ID);
    LOG_INFO("Git branch name: %s.", GIT_BRANCH_NAME);

    g_APP;

    g_APP.signal(SIGCHLD, &g_MonitorProc.onChildStatusChange);
    g_APP.signal(KILL_CHILD_PROC_SIGNAL, [](const int sig) { return true; }); // 对该信号不做处理
    
    g_APP.coreSignal([](const int){
        g_MonitorProc.stop();
        return false;
    });

    g_EventLoop.start();

    g_APP.exec();

    LOG_WARN("The APP has begun to exit.");

    g_EventLoop.stop();
    return 0;
}
