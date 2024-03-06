#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <functional>
#include <deque>
#include <map>
#include <chrono>

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


/****************************************************
 * 类说明：
 *     这个类提供定时器功能，异步任务执行功能。
 * 在本类中，有一个执行线程，用于执行提交给它的异步任务
 * 和定时任务。
 *
 * NOTE: 这里要说明的是，因为定时器的实现问题，以及操作
 * 系统本身的调度问题，它的定时器精高不会很高，当前未
 * 测试，但可以认为"在任务不繁重的时候，它的精度可能在
 * 50ms左右，随着系统任务越来越多，可能因此精度变低"。
*****************************************************/

namespace zhengze
{
    using namespace std::literals::chrono_literals;

    class EventLoop;

    class EventFD
    {
        public:
            EventFD();
            ~EventFD();

            EventFD(const EventFD&) = delete;
            EventFD(EventFD&&) = delete;
            EventFD &operator=(const EventFD&) = delete;
            EventFD &operator=(EventFD&&) = delete;

        public:
            bool open(const bool has_sem_flag = false);
            void close();
            bool isVaild()const noexcept {return event_fd_ != -1;};
            bool notify();
            bool wait();
            int fd()const noexcept{return event_fd_;};

        private:
            int event_fd_ = -1;
    };

    class TaskScheduler
    {
        friend EventLoop;
        friend std::array<TaskScheduler, TASK_SCHEDULER_COUNT>;

        class Timer
        {
            public:
                Timer();
                ~Timer();

                Timer(const Timer&) = delete;
                Timer(Timer&&) = delete;
                Timer &operator=(const Timer&) = delete;
                Timer &operator=(Timer&&) = delete;

            public:
                bool open();
                void close();
                bool isVaild()const noexcept {return epoll_fd_ != -1 && EventFD_.isVaild();};
                bool wakeup();
                bool wait(const std::chrono::milliseconds &delta_ms);

            private:
                int epoll_fd_;
                EventFD EventFD_;
        };

        public:
        TaskScheduler(const TaskScheduler&) = delete;
        TaskScheduler(TaskScheduler&&) = delete;
        TaskScheduler &operator=(const TaskScheduler&) = delete;
        TaskScheduler &operator=(TaskScheduler&&) = delete;

        public:
        // 异步任务的可调用对象。
        using AsynTask = std::function<void()>;

        // 定时任务的唯一标识
        using TimerID = uint32_t;
        // 异步任务的唯一标识
        using AsynTaskID = uint32_t;
        // 定义的无效ID
        static constexpr TimerID INVALID_TIMER_ID = 0;
        // 表示一个时间点
        using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
        //定时任务的可调用对象
        // true: 继续下一轮定时，即repeat
        // false: 完成定时任务
        using TimerTask = std::function<bool()>;

        public:
            /****************************************************
             * 函数说明：添加一个异步任务到该TaskScheduler
             * 类中的执行线程执行。该函数会立刻唤醒执行线程，
             * 以便立刻处理相应的异步任务。
             *
             * task: 需要异步执行的任务，一个可调用对象。
            *****************************************************/
            void addAsynTask(AsynTask && task);

            /****************************************************
             * 函数说明：添加一个定时任务，当时间到达时，
             * 将会在该TaskScheduler类中执行线程执行定时
             * 任务。定时任务会有精度上的问题。
             *
             * timerTask: 定时任务
             * delta_ms: 定时任务多少ms后执行
             *
             * TimerID: 添加的定时器任务对应的ID
            *****************************************************/
            TimerID addTimerTask(TimerTask &&timerTask, const std::chrono::milliseconds delta_ms);

            /****************************************************
             * 函数说明：删除一个定时任务
             *
             * timerID： 需要删除的一个定时任务的唯一ID
            *****************************************************/
            void delTimerTask(const TimerID timerID);

        private:
            /****************************************************
             * 函数说明：启动执行线程
            *****************************************************/
            bool start();

            /****************************************************
             * 函数说明：停止执行线程
            *****************************************************/
            void stop();

        private:
            /****************************************************
             * 函数说明：工具函数。通过delta_ms计算距离当前时间的时间点。
             * 即： 返回时间点 = 当前时间点 + delta_ms;
             *
             * TimePoint: 计算后的下一时间点
            *****************************************************/
            static TimePoint nextTimePoint(const std::chrono::milliseconds delta_ms);

        private:
            /****************************************************
             * 函数说明：执行线程函数
            *****************************************************/
            void scheduledThread();

        private:
            struct TimerPacket
            {
                // 定时任务
                TimerTask  timerTask;
                // 多少ms后执行
                std::chrono::milliseconds delta_ms;
            };

            struct AsynTaskPacket
            {
				// 异步任务
                AsynTask task;
				// 异步任务ID
                AsynTaskID asynTaskID;
            };

        private:
            using TimerValueType = std::pair<TimePoint, TimerID>;
            bool getNextAsynTask(AsynTaskPacket &);
            TimerValueType getNextTimerTask(std::reference_wrapper<TimerPacket> &);

        private:
            TaskScheduler();
            ~TaskScheduler();

        private:
            static uint8_t count;
            const uint8_t id_;                                 // 当前调度器ID

            std::mutex mutex_;
            Timer timer_;                                      // 任务调度器中使用的定时器。
            std::deque<AsynTaskPacket> asynTasks_;             // 一个优化:无锁队列
            AsynTaskID asynTaskID_;
            TimerID timerID_;
            std::map<TimerID, TimerPacket> timerTasks_;        // 一个优化:对于repeat定时任务，下一任务时间点的计算方式。但我们的任务要求精度不高，无所谓。
            std::multimap<TimePoint, TimerID> timers_;

            std::atomic_bool runing_;
            std::thread exec_thread_;
    };
}


#endif // TASK_SCHEDULER_H
