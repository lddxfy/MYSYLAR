#include "include/fiber.h"
#include "include/log.h"
#include "include/util.h"
#include "include/config.h"
#include "include/scheduler.h"

namespace sylar {

static Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

/// 全局静态变量，用于生成协程id
static std::atomic<uint64_t> s_fiber_id{0};
/// 全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count{0};

/// 线程局部变量，当前线程正在运行的协程
static thread_local Fiber *t_fiber = nullptr;
/// 线程局部变量，当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行，智能指针形式
static thread_local Fiber::ptr t_thread_fiber = nullptr;

//协程栈大小，可通过配置文件获取，默认128k
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size",128*1024,"fiber stack size");

/**
 * @brief malloc栈内存分配器
 */
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) {return malloc(size);}
    static void Dealloc(void* vp,size_t size){return free(vp);}
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId(){
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber(){
    SetThis(this);
    m_state = RUNNING;

    if(getcontext(&m_ctx)){
        MYSYLAR_ASSERT2(false,"getcontext");
    }

    ++s_fiber_count;
    m_id = s_fiber_id++; //协程id从0开始，用完加1

    MYSYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() main id =" << m_id;
}

void Fiber::SetThis(Fiber *f){
    t_fiber = f;
}


Fiber::Fiber(std::function<void()> cb,size_t stacksize,bool run_in_scheduler) : m_id(s_fiber_id++),m_cb(cb),m_runInScheduler(run_in_scheduler){
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);

    if(getcontext(&m_ctx)) {
        MYSYLAR_ASSERT2(false,"getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx,&Fiber::MainFunc,0); 

    MYSYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() id = " << m_id;
}

Fiber::~Fiber(){
    MYSYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id = " << m_id;
    --s_fiber_count;
    if(m_stack){
        // 有栈，说明是子协程，需要确保子协程一定是结束状态
        MYSYLAR_ASSERT(m_state == TERM);
        StackAllocator::Dealloc(m_stack,m_stacksize);
        MYSYLAR_LOG_DEBUG(g_logger) << "dealloc stack, id = " << m_id;
    }else {
        // 没有栈，说明是线程的主协程
        MYSYLAR_ASSERT(!m_cb); // 主协程没有cb
        MYSYLAR_ASSERT(m_state == RUNNING); // 主协程一定是执行状态
    }

    Fiber *cur = t_fiber;// 当前协程就是自己
    if(cur == this){
        SetThis(nullptr);
    }
}

/**
* @brief 返回当前线程正在执⾏的协程
* @details 如果当前线程还未创建协程，则创建线程的第⼀个协程，
* 且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
* 结束时,都要切回到主协程，由主协程重新选择新的协程进⾏resume
* @attention 线程如果要创建协程，那么应该⾸先执⾏⼀下Fiber::GetThis()操作，以初始化主函数协程
*/
Fiber::ptr Fiber::GetThis(){
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    MYSYLAR_ASSERT(t_fiber == main_fiber.get());
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::resume(){
    MYSYLAR_ASSERT(m_state != TERM && m_state != RUNNING)
    SetThis(this);
    m_state = RUNNING;
    // 如果协程参与调度器调度，那么应该和调度器的主协程进⾏swap，⽽不是线程主协程
    if(m_runInScheduler){
        if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx),&m_ctx)){
            MYSYLAR_ASSERT2(false,"swapcontext");
        }
    }else{
        if(swapcontext(&(t_thread_fiber->m_ctx),&m_ctx)){
            MYSYLAR_ASSERT2(false,"swapcontext");
        }
    }
    
}


void Fiber::yield(){
    MYSYLAR_ASSERT(m_state == RUNNING || m_state == TERM);
    SetThis(t_thread_fiber.get());
    if(m_state != TERM){
        m_state = READY;
    }

    if(m_runInScheduler){
        if(swapcontext(&m_ctx,&(Scheduler::GetMainFiber()->m_ctx))){
            MYSYLAR_ASSERT2(false,"swapcontext");
        }
    }else{
        if(swapcontext(&m_ctx,&(t_thread_fiber->m_ctx))){
            MYSYLAR_ASSERT2(false,"swapcontext");
        }
    }
}

void Fiber::MainFunc(){
    Fiber::ptr cur = GetThis();  // GetThis()的shared_from_this()⽅法让引⽤计数加1
    MYSYLAR_ASSERT(cur);

    cur->m_cb(); // 这⾥真正执⾏协程的⼊⼝函数
    cur->m_cb = nullptr;
    cur->m_state = TERM;

    auto raw_ptr = cur.get();  
    cur.reset();// ⼿动让t_fiber的引⽤计数减1
    raw_ptr->yield(); // 协程结束时⾃动yield，以回到主协程

}

void Fiber::reset(std::function<void()> cb) {
    MYSYLAR_ASSERT(m_stack);
    MYSYLAR_ASSERT(m_state == TERM);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        MYSYLAR_ASSERT2(false,"getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx,&Fiber::MainFunc,0);
    m_state = READY;
}

}