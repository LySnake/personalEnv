/*
 * @Description: Copyright (C) SHENZHEN JoyLife Robot Co.
 * @version: v0.01
 * @Author: litaobo
 * @Date: 2023-03-23 11:04:28
 * @LastEditors: litaobo
 * @LastEditTime: 2023-03-23 11:28:01
 */
#include <adk/utils/logger.h>

#include "EventLoop.h"

#undef TAG
#define TAG "common.EventLoop"

namespace joyrobot
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
