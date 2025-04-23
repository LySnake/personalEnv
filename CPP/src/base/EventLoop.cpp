
#include "EventLoop.h"
#include "log.h"
#include "os.h"

EventLoop &EventLoop::get_instance()
{
    static EventLoop ins;
    return ins;
}

void EventLoop::start()
{
    for (auto &scheduler : taskSchedulers_)
    {
        if (!scheduler.start())
        {
            LOG_ERROR("Failed to start EventLoop.");
            exit(1);
        }
    }
}

void EventLoop::stop()
{
    for (auto &scheduler : taskSchedulers_)
    {
        scheduler.stop();
    }
}

TaskScheduler &EventLoop::get(const uint8_t id)
{
    if (id >= taskSchedulers_.size())
    {
        LOG_ERROR("out of rang. id:%d.", id);
        exit(1);
    }

    return taskSchedulers_[id];
}
