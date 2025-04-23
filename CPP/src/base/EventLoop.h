#ifndef EVENT_POOL_H
#define EVENT_POOL_H

#include <array>

#include "TaskScheduler.h"

#define g_EventLoop EventLoop::get_instance()

// 主线程(调度器)
#define MASTER_SCHEDULER 0
// 副线程(调度器)
#define SLAVE_SCHEDULER 1

#if TASK_SCHEDULER_COUNT > MASTER_SCHEDULER
#define g_MasterScheduler g_EventLoop.get(MASTER_SCHEDULER)
#endif

#if TASK_SCHEDULER_COUNT > SLAVE_SCHEDULER
#define g_SlaveScheduler g_EventLoop.get(SLAVE_SCHEDULER)
#endif

class EventLoop
{
  public:
    EventLoop(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    EventLoop &operator=(EventLoop &&) = delete;

    static EventLoop &get_instance();

  public:
    void start();
    void stop();

    TaskScheduler &get(const uint8_t id);

  private:
    EventLoop() = default;
    ~EventLoop() = default;

  private:
    std::array<TaskScheduler, TASK_SCHEDULER_COUNT> taskSchedulers_;
};

#endif // EVENT_POOL_H
