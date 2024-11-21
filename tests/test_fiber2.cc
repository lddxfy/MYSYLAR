
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

    /**
     * 非对称协程，子协程不能创建并运行新的子协程，下面的操作是有问题的，
     * 子协程再创建子协程，原来的主协程就跑飞了
     */
    lamb::Fiber::ptr fiber(new lamb::Fiber(run_in_fiber2, 0, false));
    fiber->resume();

    LAMB_LOG_INFO(g_logger) << "run_in_fiber end";
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    LAMB_LOG_INFO(g_logger) << "main begin";

    lamb::Fiber::GetThis();

    lamb::Fiber::ptr fiber(new lamb::Fiber(run_in_fiber, 0, false));
    fiber->resume();

    LAMB_LOG_INFO(g_logger) << "main end";
    return 0;
}