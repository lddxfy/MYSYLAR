
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

    /**
     * 非对称协程，子协程不能创建并运行新的子协程，下面的操作是有问题的，
     * 子协程再创建子协程，原来的主协程就跑飞了
     */
    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber2, 0, false));
    fiber->resume();

    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
}

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    MYSYLAR_LOG_INFO(g_logger) << "main begin";

    sylar::Fiber::GetThis();

    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber, 0, false));
    fiber->resume();

    MYSYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}