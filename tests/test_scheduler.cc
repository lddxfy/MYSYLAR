#include "../include/log.h"
#include "../include/fiber.h"
#include "../include/env.h"
#include "../include/config.h"
#include "../include/util.h"
#include "../include/scheduler.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

/**
 * @brief 演示协程主动yield情况下应该如何操作
 */
void test_fiber1() {
    LAMB_LOG_INFO(g_logger) << "test_fiber1 begin";
  
    /**
     * 协程主动让出执行权，在yield之前，协程必须再次将自己添加到调度器任务队列中，
     * 否则yield之后没人管，协程会处理未执行完的逃逸状态，测试时可以将下面这行注释掉以观察效果
     */
    lamb::Scheduler::GetThis()->schedule(lamb::Fiber::GetThis());

    LAMB_LOG_INFO(g_logger) << "before test_fiber1 yield";
    lamb::Fiber::GetThis()->yield();
    LAMB_LOG_INFO(g_logger) << "after test_fiber1 yield";

    LAMB_LOG_INFO(g_logger) << "test_fiber1 end";
}

/**
 * @brief 演示协程睡眠对主程序的影响
 */
void test_fiber2() {
    LAMB_LOG_INFO(g_logger) << "test_fiber2 begin";

    /**
     * 一个线程同一时间只能有一个协程在运行，线程调度协程的本质就是按顺序执行任务队列里的协程
     * 由于必须等一个协程执行完后才能执行下一个协程，所以任何一个协程的阻塞都会影响整个线程的协程调度，这里
     * 睡眠的3秒钟之内调度器不会调度新的协程，对sleep函数进行hook之后可以改变这种情况
     */
    sleep(3);

    LAMB_LOG_INFO(g_logger) << "test_fiber2 end";
}

void test_fiber3() {
    LAMB_LOG_INFO(g_logger) << "test_fiber3 begin";
    LAMB_LOG_INFO(g_logger) << "test_fiber3 end";
}

void test_fiber5() {
    static int count = 0;

    LAMB_LOG_INFO(g_logger) << "test_fiber5 begin, i = " << count;
    LAMB_LOG_INFO(g_logger) << "test_fiber5 end i = " << count;

    count++;
}

/**
 * @brief 演示指定执行线程的情况
 */
void test_fiber4() {
    LAMB_LOG_INFO(g_logger) << "test_fiber4 begin";
    
    for (int i = 0; i < 3; i++) {
        lamb::Scheduler::GetThis()->schedule(test_fiber5, lamb::GetThreadId());
    }

    LAMB_LOG_INFO(g_logger) << "test_fiber4 end";
}

int main(int argc,char *argv[]) {

    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());
    LAMB_LOG_INFO(g_logger) << "main begin";

    /** 
     * 只使用main函数线程进行协程调度，相当于先攒下一波协程，然后切换到调度器的run方法将这些协程
     * 消耗掉，然后再返回main函数往下执行
     */
    //lamb::Scheduler sc;

    // 额外创建新的线程进行调度，那只要添加了调度任务，调度器马上就可以调度该任务
    lamb::Scheduler sc(3, false);

    // 添加调度任务，使用函数作为调度对象
    sc.schedule(test_fiber1);
    sc.schedule(test_fiber2);

    // 添加调度任务，使用Fiber类作为调度对象
    lamb::Fiber::ptr fiber(new lamb::Fiber(&test_fiber3));
    sc.schedule(fiber);

    // 创建调度线程，开始任务调度，如果只使用main函数线程进行调度，那start相当于什么也没做
    sc.start();

    /**
     * 只要调度器未停止，就可以添加调度任务
     * 包括在子协程中也可以通过lamb::Scheduler::GetThis()->scheduler()的方式继续添加调度任务
     */
    sc.schedule(test_fiber4);

    /**
     * 停止调度，如果未使用当前线程进行调度，那么只需要简单地等所有调度线程退出即可
     * 如果使用了当前线程进行调度，那么要先执行当前线程的协程调度函数，等其执行完后再返回caller协程继续往下执行
     */
    sc.stop();

    LAMB_LOG_INFO(g_logger) << "main end";
    return 0;
}