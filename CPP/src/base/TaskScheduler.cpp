#include <cstdlib>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <string.h>

#include "TaskScheduler.h"
#include "log.h"
#include "os.h"

uint8_t TaskScheduler::count = 0;

EventFD::EventFD() : event_fd_{-1} {}

EventFD::~EventFD() { EventFD::close(); }

bool EventFD::open(const bool has_sem_flag)
{
    if (isVaild())
    {
        LOG_ERROR("Repeat to create event_fd.");
        return false;
    }

    if (has_sem_flag)
    {
        event_fd_ = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
    }
    else
    {
        event_fd_ = eventfd(0, EFD_CLOEXEC);
    }

    if (!isVaild())
    {
        LOG_ERROR("create event_fd fail. %s.", os::POSIX_errno().c_str());
        return false;
    }

    return true;
}

void EventFD::close()
{
    if (isVaild())
    {
        ::close(event_fd_);
        event_fd_ = -1;
    }
}

bool EventFD::notify()
{
    if (!isVaild())
    {
        LOG_ERROR("The event_fd is invalid.");
        return false;
    }

    const eventfd_t eventfd_value = 1;
    if (eventfd_write(event_fd_, eventfd_value))
    {
        LOG_ERROR("Failed to write the eventfd_value to event_fd. %s.", os::POSIX_errno().c_str());
        return false;
    }

    return true;
}

bool EventFD::wait()
{
    if (!isVaild())
    {
        LOG_ERROR("The event_fd is invalid.");
        return false;
    }

    eventfd_t eventfd_value = 0;
    if (eventfd_read(event_fd_, &eventfd_value))
    {
        LOG_ERROR("Failed to read the eventfd_value from event_fd. %s.", os::POSIX_errno().c_str());
        return false;
    }

    return true;
}

TaskScheduler::Timer::Timer() : epoll_fd_{-1} {}

TaskScheduler::Timer::~Timer() { Timer::close(); }

bool TaskScheduler::Timer::open()
{
    if (isVaild())
    {
        LOG_ERROR("Repeat to create epoll_fd.");
        return false;
    }

    epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1)
    {
        LOG_ERROR("Failed to create epoll_fd. %s.", os::POSIX_errno().c_str());
        return false;
    }

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;

    if (!EventFD_.open() || ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, EventFD_.fd(), &event))
    {
        LOG_ERROR("Failed to associated event_fd. %s.", os::POSIX_errno().c_str());
        return false;
    }

    return true;
}

void TaskScheduler::Timer::close()
{
    if (isVaild())
    {
        wakeup();
        ::close(epoll_fd_);
        epoll_fd_ = -1;
        EventFD_.close();
    }
}

bool TaskScheduler::Timer::wakeup() { return isVaild() && EventFD_.notify(); }

bool TaskScheduler::Timer::wait(const std::chrono::milliseconds &delta_ms)
{
    if (!isVaild())
    {
        LOG_ERROR("The Timer is invalid.");
        return false;
    }

    struct epoll_event event;
    auto ret = ::epoll_wait(epoll_fd_, &event, 1, delta_ms.count());
    if (ret == -1)
    {
        if (errno != EINTR)
        {
            LOG_ERROR("Failed to invoke epoll_wait. %s.", os::POSIX_errno().c_str());
            return false;
        }

        return true;
    }
    else if (ret == 0) // 超时
    {
        return true;
    }

    return ret > 0 && EventFD_.wait();
}

TaskScheduler::TaskScheduler() : id_{count++}, asynTaskID_{0}, timerID_{INVALID_TIMER_ID}, runing_{false} {}

TaskScheduler::~TaskScheduler()
{
    stop();
    count--;
}

bool TaskScheduler::start()
{
    if (runing_.exchange(true) == false && exec_thread_.joinable() == false)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!timer_.open())
        {
            runing_ = false;
            return false;
        }
        exec_thread_ = std::thread(&TaskScheduler::scheduledThread, this);
    }
    return true;
}

void TaskScheduler::stop()
{
    if (runing_.exchange(false) && exec_thread_.joinable())
    {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            timer_.wakeup();
        }
        exec_thread_.join();
        {
            std::lock_guard<std::mutex> guard(mutex_);
            timer_.close();
        }
    }
}

void TaskScheduler::addAsynTask(AsynTask &&task)
{
    AsynTaskID asynTaskID = 0;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        asynTaskID = ++asynTaskID_;
        asynTasks_.emplace_back(AsynTaskPacket{std::move(task), asynTaskID});
        timer_.wakeup();
    }
    // LOG_DEBUG("addAsynTask. Scheduler ID:%u, asynTaskID: %u.", id_, asynTaskID);
}

TaskScheduler::TimerID TaskScheduler::addTimerTask(TimerTask &&timerTask, const std::chrono::milliseconds delta_ms)
{
    TimerID timerID = 0;
    TimerPacket task = {std::move(timerTask), delta_ms};
    {
        std::lock_guard<std::mutex> guard(mutex_);

        timerID = ++timerID_;
        auto timePoint = nextTimePoint(task.delta_ms);
        timerTasks_.emplace(timerID, std::move(task));
        timers_.emplace(timePoint, timerID);
        timer_.wakeup();
    }

    // LOG_DEBUG("addTimerTask. Scheduler ID:%u, timerID: %u.", id_, timerID);
    return timerID;
}

void TaskScheduler::delTimerTask(const TimerID timerID)
{
    {
        std::lock_guard<std::mutex> guard(mutex_);
        timerTasks_.erase(timerID);
    }
}

bool TaskScheduler::getNextAsynTask(AsynTaskPacket &asynTaskPacket)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!asynTasks_.empty())
    {
        asynTaskPacket = std::move(*(asynTasks_.begin()));
        asynTasks_.pop_front();
        return true;
    }

    return false;
}

TaskScheduler::TimerValueType TaskScheduler::getNextTimerTask(std::reference_wrapper<TimerPacket> &timerPacket)
{
    TimerValueType retTimer{TimePoint::min(), INVALID_TIMER_ID};
    std::lock_guard<std::mutex> guard(mutex_);
    if (!timers_.empty())
    {
        auto iter = timers_.begin();
        retTimer = *iter;
        timers_.erase(iter);

        auto taskIter = timerTasks_.find(retTimer.second);
        if (taskIter == timerTasks_.end())
        {
            timerPacket.get().timerTask = TimerTask{};
        }
        else
        {
            timerPacket = std::ref<TimerPacket>(taskIter->second);
        }
    }

    return retTimer;
}

void TaskScheduler::scheduledThread()
{
    LOG_INFO("function: %s, Scheduler ID:%u. enter.", __FUNCTION__, id_);

    while (runing_)
    {
        AsynTaskID asynTaskID = 0;
        UNUSED(asynTaskID);
        try
        {
            // 处理异步任务
            AsynTaskPacket asynTaskPacket;
            while (getNextAsynTask(asynTaskPacket))
            {
                if (asynTaskPacket.task)
                {
                    asynTaskID = asynTaskPacket.asynTaskID;
                    auto start = std::chrono::steady_clock::now();
                    asynTaskPacket.task();
                    auto end = std::chrono::steady_clock::now();

                    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    if (delta > 50) // 50ms
                    {
                        LOG_WARN("Asyntask duration: %.1fms. Scheduler ID: %u. asynTaskID: %u.", (end - start) / 1e6,
                                 id_, asynTaskID);
                    }
                    else
                    {
                        // LOG_DEBUG("Asyntask done. Scheduler ID:%u, asynTaskID: %u.", id_, asynTaskID);
                    }
                }
                asynTaskID = 0;
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("AsynTask. Catch CXX std::exception: %s. Scheduler ID: %u. asynTaskID: %u.", e.what(), id_,
                      asynTaskID);
            std::abort();
        }
        catch (...)
        {
            LOG_ERROR("AsynTask. Catch CXX unknow exception. Scheduler ID: %u. asynTaskID: %u.", id_, asynTaskID);
            std::abort();
        }

        std::chrono::milliseconds needSleepTime_ms{60 * 1000};
        auto nowPoint = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
        TimerID timerID = INVALID_TIMER_ID;
        UNUSED(timerID);
        try
        {
            while (true)
            {
                TimerPacket tempObj;
                auto timerPacket = std::ref<TimerPacket>(tempObj);
                auto timerEle = getNextTimerTask(timerPacket);

                // 无定时任务
                if (timerEle.second == INVALID_TIMER_ID)
                {
                    break;
                }
                // 无需要执行的定时任务
                else if (timerEle.first > nowPoint)
                {
                    needSleepTime_ms = timerEle.first - nowPoint;

                    std::lock_guard<std::mutex> lock(mutex_);
                    timers_.emplace(timerEle);
                    break;
                }
                // 处理定时任务
                else
                {
                    if (timerPacket.get().timerTask)
                    {
                        timerID = timerEle.second;
                        auto start = std::chrono::steady_clock::now();
                        if (timerPacket.get().timerTask())
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            // repeat定时任务
                            timers_.emplace(nextTimePoint(timerPacket.get().delta_ms), timerEle.second);
                        }
                        else
                        {
                            // 结束定时任务
                            delTimerTask(timerEle.second);
                        }
                        auto end = std::chrono::steady_clock::now();
                        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                        if (delta > 50) // 50ms
                        {
                            LOG_WARN("Timertask duration: %.1fms. Scheduler ID: %u.timerID: %u.", (end - start) / 1e6,
                                     id_, timerID);
                        }
                        else
                        {
                            // LOG_DEBUG("Timertask done. Scheduler ID:%u, timerID: %u.", id_, timerID);
                        }
                    }
                    else
                    {
                        delTimerTask(timerEle.second);
                    }
                    timerID = INVALID_TIMER_ID;
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("TimerTask. Catch CXX std::exception: %s. Scheduler ID: %u. timerID: %u.", e.what(), id_,
                      timerID);
            std::abort();
        }
        catch (...)
        {
            LOG_ERROR("TimerTask. Catch CXX unknow exception. Scheduler ID: %u. timerID: %u.", id_, timerID);
            std::abort();
        }

        if (runing_)
        {
            if (!timer_.wait(needSleepTime_ms))
            {
                LOG_ERROR("function: %s. An unexpected failure. ", __FUNCTION__);
            }
        }
    }

    LOG_INFO("function: %s, Scheduler ID: %u. out.", __FUNCTION__, id_);
}

TaskScheduler::TimePoint TaskScheduler::nextTimePoint(const std::chrono::milliseconds delta_ms)
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()) + delta_ms;
}
