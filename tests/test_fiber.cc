
#include <log.h>
#include <fiber.h>
#include <env.h>
#include <config.h>
#include <util.h>
#include <string>
#include <vector>

sylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void run_in_fiber2() {
    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber2 begin";
    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber() {
    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";

    MYSYLAR_LOG_INFO(g_logger) << "before run_in_fiber yield";
    sylar::Fiber::GetThis()->yield();
    MYSYLAR_LOG_INFO(g_logger) << "after run_in_fiber yield";

    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
    // fiber结束之后会自动返回主协程运行
}

void test_fiber() {
    MYSYLAR_LOG_INFO(g_logger) << "test_fiber begin";

    // 初始化线程主协程
    sylar::Fiber::GetThis();

    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber, 0, false));
    MYSYLAR_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1

    MYSYLAR_LOG_INFO(g_logger) << "before test_fiber resume";
    fiber->resume();
    MYSYLAR_LOG_INFO(g_logger) << "after test_fiber resume";

    /** 
     * 关于fiber智能指针的引用计数为3的说明：
     * 一份在当前函数的fiber指针，一份在MainFunc的cur指针
     * 还有一份在在run_in_fiber的GetThis()结果的临时变量里
     */
    MYSYLAR_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 3

    MYSYLAR_LOG_INFO(g_logger) << "fiber status: " << fiber->getState(); // READY

    MYSYLAR_LOG_INFO(g_logger) << "before test_fiber resume again";
    fiber->resume();
    MYSYLAR_LOG_INFO(g_logger) << "after test_fiber resume again";

    MYSYLAR_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1
    MYSYLAR_LOG_INFO(g_logger) << "fiber status: " << fiber->getState(); // TERM

    fiber->reset(run_in_fiber2); // 上一个协程结束之后，复用其栈空间再创建一个新协程
    fiber->resume();

    MYSYLAR_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1
    MYSYLAR_LOG_INFO(g_logger) << "test_fiber end";
}

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    sylar::SetThreadName("main_thread");
    MYSYLAR_LOG_INFO(g_logger) << "main begin";

    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 2; i++) {
        thrs.push_back(sylar::Thread::ptr(
            new sylar::Thread(&test_fiber, "thread_" + std::to_string(i))));
    }

    for (auto i : thrs) {
        i->join();
    }

    MYSYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}