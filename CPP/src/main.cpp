
#include "App.h"
#include "EventLoop.h"
#include "MonitorProc.h"
#include "log.h"

int main()
{
    g_APP;

    g_APP.signal(SIGCHLD, &g_MonitorProc.onChildStatusChange);
    g_APP.signal(KILL_CHILD_PROC_SIGNAL, [](const int sig) { return true; }); // 对该信号不做处理

    LOG_INFO("Build Time: %s %s.", __DATE__, __TIME__);
    LOG_INFO("Git commit id: %s.", GIT_COMMIT_ID);
    LOG_INFO("Git branch name: %s.", GIT_BRANCH_NAME);

    g_EventLoop.start();

    g_APP.exec();

    g_EventLoop.stop();
    return 0;
}
