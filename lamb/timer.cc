#include "include/timer.h"
#include "include/util.h"

namespace lamb
{

    bool Timer::Comparator::operator()(const Timer::ptr& lhs,const Timer::ptr& rhs) const
    {
        if(!lhs && !rhs){
            return false;
        }
        if(!lhs){
            return true;
        }
        if(!rhs){
            return false;
        }
        if(lhs->m_next < rhs->m_next){
            return true;
        }
        if(rhs->m_next < lhs->m_next){
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        :m_recurring(recurring)
        ,m_ms(ms)
        ,m_cb(cb)
        ,m_manager(manager)
    {
        m_next = lamb::GetElapsedMS() + m_ms;
    }

    Timer::Timer(uint64_t next) : m_next(next)
    {
    }

    // 取消定时器
    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(m_cb){
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    // 刷新设置定时器的执行时间
    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }
        m_manager->m_timers.erase(it);
        m_next = lamb::GetElapsedMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    // 重置定时器时间
    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if(ms == m_ms && !from_now){
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        //从现在开始重新计时
        if(from_now){
            start = lamb::GetElapsedMS();
        }else {
            //从之前设置的时间开始也就是上一个lamb::GetElapsedMS()
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = m_ms+start;
        m_manager->addTimer(shared_from_this(),lock);
        return true;
    }

    TimerManager::TimerManager()
    {
        m_previouseTime = lamb::GetElapsedMS();
    }

    TimerManager::~TimerManager()
    {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms,cb,recurring,this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer,lock);
        return timer;
    }

    static void OnTimer(std::weak_ptr<void> weak_cond,std::function<void()> cb) {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if(tmp){
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms,std::bind(&OnTimer,weak_cond,cb),recurring);
    }

    // 到最近一个定时器执行的时间间隔
    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if(m_timers.empty()){
            return ~0ull;
        }

        const Timer::ptr& next = *m_timers.begin();
        uint64_t now_ms = lamb::GetElapsedMS();
        if(now_ms >= next->m_next){
            return 0;
        } else {
            return next->m_next-now_ms;
        }
    }
    /**
     * @brief 获取需要执行的定时器的回调函数列表
     * @param[out] cbs 回调函数数组
     */
    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = lamb::GetElapsedMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(m_timers.empty()){
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        if(m_timers.empty()){
            return;
        }
        bool rollover = false;
        if(LAMB_UNLIKELY(detectClockRollover(now_ms))) {
            // 使用clock_gettime(CLOCK_MONOTONIC_RAW)，应该不可能出现时间回退的问题
            rollover = true;
        }
        //没有时间回退且当前没有定时器超时，直接返回
        if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        //如果没有时间回滚，使用lower_bound(now_timer)找到第一个未过期的定时器，否则从容器末尾开始搜索
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while(it != m_timers.end() && (*it)->m_next == now_ms){
            ++it;
        }
        //将所有已过期的定时器（即触发时间等于当前时间的定时器）收集到expired向量中
        expired.insert(expired.begin(),m_timers.begin(),it);
        m_timers.erase(m_timers.begin(),it);
        cbs.reserve(expired.size());

        for(auto& timer : expired){
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring){
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }else{
                timer->m_cb = nullptr;
            }
        }
    }

    bool TimerManager::hasTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

    //将定时器添加到管理器中
    void TimerManager::addTimer(Timer::ptr val,RWMutexType::WriteLock& lock)
    {
        //将val插入到m_timers中，并获取指向集合中该元素位置的迭代器
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if(at_front){
            m_tickled = true;
        }
        lock.unlock();

        if(at_front){
            onTimerInsertedAtFront();
        }

    }

    //检测服务器时间是否被调后了
    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if(now_ms < m_previouseTime && now_ms < (m_previouseTime - 60*60*1000)){
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }

}