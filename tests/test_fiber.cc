
#include "../include/log.h"
#include "../include/fiber.h"
#include "../include/env.h"
#include "../include/config.h"
#include "../include/util.h"
#include <string>
#include <vector>

lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

void run_in_fiber2() {
    LAMB_LOG_INFO(g_logger) << "run_in_fiber2 begin";
    LAMB_LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber() {
    LAMB_LOG_INFO(g_logger) << "run_in_fiber begin";

    LAMB_LOG_INFO(g_logger) << "before run_in_fiber yield";
    lamb::Fiber::GetThis()->yield();
    LAMB_LOG_INFO(g_logger) << "after run_in_fiber yield";

    LAMB_LOG_INFO(g_logger) << "run_in_fiber end";
    // fiber结束之后会自动返回主协程运行
}

void test_fiber() {
    LAMB_LOG_INFO(g_logger) << "test_fiber begin";

    // 初始化线程主协程
    lamb::Fiber::GetThis();

    lamb::Fiber::ptr fiber(new lamb::Fiber(run_in_fiber, 0, false));
    LAMB_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1

    LAMB_LOG_INFO(g_logger) << "before test_fiber resume";
    fiber->resume();
    LAMB_LOG_INFO(g_logger) << "after test_fiber resume";

    /** 
     * 关于fiber智能指针的引用计数为3的说明：
     * 一份在当前函数的fiber指针，一份在MainFunc的cur指针
     * 还有一份在在run_in_fiber的GetThis()结果的临时变量里
     */
    LAMB_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 3

    LAMB_LOG_INFO(g_logger) << "fiber status: " << fiber->getState(); // READY

    LAMB_LOG_INFO(g_logger) << "before test_fiber resume again";
    fiber->resume();
    LAMB_LOG_INFO(g_logger) << "after test_fiber resume again";

    LAMB_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1
    LAMB_LOG_INFO(g_logger) << "fiber status: " << fiber->getState(); // TERM

    fiber->reset(run_in_fiber2); // 上一个协程结束之后，复用其栈空间再创建一个新协程
    fiber->resume();

    LAMB_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1
    LAMB_LOG_INFO(g_logger) << "test_fiber end";
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    lamb::SetThreadName("main_thread");
    LAMB_LOG_INFO(g_logger) << "main begin";

    std::vector<lamb::Thread::ptr> thrs;
    for (int i = 0; i < 2; i++) {
        thrs.push_back(lamb::Thread::ptr(
            new lamb::Thread(&test_fiber, "thread_" + std::to_string(i))));
    }

    for (auto i : thrs) {
        i->join();
    }

    LAMB_LOG_INFO(g_logger) << "main end";
    return 0;
}