#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__
#include "scheduler.h"
#include "timer.h"
#include <memory>
namespace sylar{

class IOManager : public Scheduler,public TimerManager{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE = 0x0, //无事件
        READ = 0x1, //读事件（EPOLLIN）
        WRITE = 0x4,//写事件（EPOLLOUT）
    };
private:
    /**
    * @brief socket fd上下⽂类
    * @details 每个socket fd都对应⼀个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下⽂
    */
    struct FdContext
    {
        /* data */
        typedef Mutex MutexType;
        /**
        * @brief 事件上下⽂类
        * @details fd的每个事件都有⼀个事件上下⽂，保存这个事件的回调函数以及执⾏回调函数的调度器
        * sylar对fd事件做了简化，只预留了读事件和写事件，所有的事件都被归类到这两类事件中
        */
        struct EventContext
        {
            /* data */
            //执行事件回调的调度器
            Scheduler *scheduler = nullptr;
            //事件回调协程
            Fiber::ptr fiber;
            //事件回调函数
            std::function<void()> cb;
        };

        //获取事件上下文类
        EventContext& getEventContext(Event event);

        //重置事件上下文
        void resetEventContext(EventContext& ctx);

        /**
        * @brief 触发事件
        * @details 根据事件类型调⽤对应上下⽂结构中的调度器去调度回调协程或回调函数
        * @param[in] event 事件类型
        */
       void triggerEvent(Event event);

        //读事件上下文
       EventContext read;
       //写事件上下文
       EventContext write;
       //事件关联的句柄
       int fd = 0;
       //该fd添加了哪些事件的回调函数，或者说该fd关心哪些事件
       Event events = NONE;
       //事件的Mutex
       MutexType mutex; 
        
    };

    void contextResize(size_t size);
        
public:
    IOManager(size_t threads = 1,bool use_caller = true,const std::string &name = "IOManager");

    ~IOManager();

    int addEvent(int fd,Event event,std::function<void()> cb = nullptr);

    bool delEvent(int fd,Event event);

    bool cancelEvent(int fd,Event event);
    //取消该fd上注册的所有事情，同时也要触发一次
    bool cancelAll(int fd);

    static IOManager* GetThis();
protected:
    void tickle() override;

    bool stopping() override;
    /**
    * @brief 判断是否可以停止，同时获取最近一个定时器的超时时间
    * @param[out] timeout 最近一个定时器的超时时间，用于idle协程的epoll_wait
    * @return 返回是否可以停止
    */
    bool stopping(uint64_t& timeout);

    //当有定时器插入到头部时，要重新更新epoll_wait的超时时间，这里时唤醒idle协程以便于使用新的超时时间
    void onTimerInsertedAtFront() override;


    void idle() override;

private:
    //epoll 文件句柄
    int m_epfd = 0;
    //pipe 文件句柄，fd[0] 读端，fd[1]写端
    int m_tickleFds[2];
    //当前等待执行的IO事件数量
    std::atomic<size_t>m_pendingEventCount = {0};
    //IOManager的Mutex
    RWMutexType m_mutex;
    //socket事件上下文的容器
    std::vector<FdContext *> m_fdContexts;


    
};

}


#endif