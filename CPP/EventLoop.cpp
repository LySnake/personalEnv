
#include "utils.h"
#include "EventLoop.h"

namespace zhengze
{
    EventLoop & EventLoop::get_instance()
    {
        static EventLoop ins;
        return ins;
    }

    void EventLoop::start()
    {
        for(auto &scheduler: taskSchedulers_)
        {
            if(!scheduler.start())
            {
                pr_err("Failed to start EventLoop.");
                exit(1);
            }
        }
    }

    void EventLoop::stop()
    {
        for(auto &scheduler: taskSchedulers_)
        {
            scheduler.stop();
        }
    }

    TaskScheduler& EventLoop::get(const uint8_t id)
    {
        if(id >= taskSchedulers_.size())
        {
            pr_err("out of rang. id:%d.", id);
            exit(1);
        }

        return taskSchedulers_[id];
    }

}
