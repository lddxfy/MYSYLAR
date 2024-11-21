#include "include/scheduler.h"
#include "include/log.h"
#include "include/hook.h"

namespace sylar
{
    // 当前线程的调度器，同一个调度器下的所有线程指向同一个调度器实例
    static thread_local Scheduler *t_scheduler = nullptr;
    // 当前线程的调度协程，每个线程都独有一份，包括caller线程
    static thread_local Fiber *t_scheduler_fiber = nullptr;

    static Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    {
        MYSYLAR_ASSERT(threads > 0);

        m_useCaller = use_caller;
        m_name = name;

        if (use_caller)
        {
            --threads;
            sylar::Fiber::GetThis();
            MYSYLAR_ASSERT(GetThis() == nullptr);
            t_scheduler = this;

            /**
             * 在user_caller为true的情况下，初始化caller线程的调度协程
             * caller线程的调度协程不会被调度器调度，⽽且，caller线程的调度协程停⽌时，应该返回caller线程的主协程
             **/

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
            sylar::Thread::SetName(m_name);
            t_scheduler_fiber = m_rootFiber.get();
            m_rootThread = sylar::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }

    Fiber *Scheduler::GetMainFiber(){
        return t_scheduler_fiber;
    }

    void Scheduler::setThis(){
        t_scheduler = this;

    }

    Scheduler::~Scheduler(){
        MYSYLAR_LOG_DEBUG(g_logger) << "Scheduler::~Schedule()";
        MYSYLAR_ASSERT(m_stopping);
        if(GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    bool Scheduler::stopping(){
        MutexType::Lock lock(m_mutex);
        return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::tickle(){
        MYSYLAR_LOG_DEBUG(g_logger) << "ticlke";
    }

    void Scheduler::idle(){
        MYSYLAR_LOG_DEBUG(g_logger) << "idle";
        while(!stopping()){
            sylar::Fiber::GetThis()->yield();
        }
    }

    // 这⾥主要初始化调度线程池，如果只使⽤caller线程进⾏调度，那这个⽅法啥也不做
    void Scheduler::start()
    {
        MYSYLAR_LOG_DEBUG(g_logger) << "start";
        MutexType::Lock lock(m_mutex);
        if (m_stopping)
        {
            MYSYLAR_LOG_ERROR(g_logger) << "Scheduler is stopped";
            return;
        }

        MYSYLAR_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; i++)
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    void Scheduler::run()
    {
        MYSYLAR_LOG_DEBUG(g_logger) << "run";
        set_hook_enable(true);
        setThis();
        if (sylar::GetThreadId() != m_rootThread)
        {
            t_scheduler_fiber = sylar::Fiber::GetThis().get();
        }

        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        ScheduleTask task;
        while (true)
        {
            task.reset();
            bool tickle_me = false; // 是否tickle其他线程进⾏任务调度
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_tasks.begin();
                while (it != m_tasks.end())
                {
                    if (it->thread != -1 && it->thread != sylar::GetThreadId())
                    {
                        // 指定了调度线程，但不是在当前线程上调度，标记⼀下需要通知其他线程进⾏调度，然后跳过这个任务，继续下⼀个
                        ++it;
                        tickle_me = true;
                        continue;
                    }
                    // 找到⼀个未指定线程，或是指定了当前线程的任务
                    MYSYLAR_ASSERT(it->fiber || it->cb);
                    if (it->fiber)
                    {
                        // 任务队列时的协程⼀定是READY状态，谁会把RUNNING或TERM状态的协程加⼊调度呢？
                        MYSYLAR_ASSERT(it->fiber->getState() == Fiber::READY);
                    }
                    // 当前调度线程找到一个任务，准备开始调度。将其从任务队列中剔除，活动线程数加1
                    task = *it;
                    m_tasks.erase(it++);
                    ++m_activeThreadCount;
                    break;
                }
                // 当前线程拿完⼀个任务后，发现任务队列还有剩余，那么tickle⼀下其他线程
                tickle_me |= (it != m_tasks.end());
            }
            if (tickle_me)
            {
                tickle();
            }

            if (task.fiber)
            {
                // resume协程，resume返回时，协程要么执⾏完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减⼀
                task.fiber->resume();
                --m_activeThreadCount;
                task.reset();
            }
            else if (task.cb)
            {
                if (cb_fiber)
                {
                    cb_fiber->reset(task.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(task.cb));
                }
                task.reset();
                cb_fiber->resume();
                --m_activeThreadCount;
                cb_fiber.reset();
            }
            else
            {
                // 进到这个分⽀情况⼀定是任务队列空了，调度idle协程即可
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那⼀定是调度器停⽌了
                    MYSYLAR_LOG_DEBUG(g_logger) << "idle fiber term";
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->resume();
                --m_idleThreadCount;
            }
        }
        MYSYLAR_LOG_DEBUG(g_logger) << "Scheduler::run() exit";
    }

    void Scheduler::stop()
    {
        MYSYLAR_LOG_DEBUG(g_logger) << "stop";
        if (stopping())
        {
            return;
        }
        m_stopping = true;
        /// 如果use caller，那只能由caller线程发起stop
        if (m_useCaller)
        {
            MYSYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            MYSYLAR_ASSERT(GetThis() != this);
        }
        for (size_t i = 0; i < m_threadCount; i++)
        {
            tickle();
        }
        if (m_rootFiber)
        {
            tickle();
        }
        /// 在use caller情况下，调度器协程结束时，应该返回caller协程
        if (m_rootFiber)
        {
            m_rootFiber->resume();
            MYSYLAR_LOG_DEBUG(g_logger) << "m_rootFiber end";
        }
        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        for (auto &i : thrs)
        {
            i->join();
        }
    }

}