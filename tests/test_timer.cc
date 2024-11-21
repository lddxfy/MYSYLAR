/**
 * @file test_timer.cc
 * @brief IO协程测试器定时器测试
 * @version 0.1
 * @date 2021-06-19
 */

#include "../include/iomanager.h"
#include "../include/log.h"
#include "../include/config.h"
#include "../include/env.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

static int timeout = 1000;
static lamb::Timer::ptr s_timer;

struct timer_info {
    int cancelled = 0;
};

void timer_callback() {
    LAMB_LOG_INFO(g_logger) << "timer callback, timeout = " << timeout;
    timeout += 1000;
    if(timeout < 5000) {
        s_timer->reset(timeout, true);
    } else {
        s_timer->cancel();
    }
}

void test_timer() {
    lamb::IOManager iom;

    // 循环定时器
    // s_timer = iom.addTimer(1000, timer_callback, true);
    
    // 单次定时器
    auto timer = iom.addTimer(1000, []{
        LAMB_LOG_INFO(g_logger) << "500ms timeout";
    });
    // std::shared_ptr<timer_info> tinfo(new timer_info);
    // std::weak_ptr<timer_info> winfo(tinfo);
    // iom.addConditionTimer(1000,[](){
    //     LAMB_LOG_INFO(g_logger) << "1000ms timeout";

    // },winfo);

    
    // iom.addTimer(5000, []{
    //     LAMB_LOG_INFO(g_logger) << "5000ms timeout";
    // });
    timer->cancel();
    
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    test_timer();

    LAMB_LOG_INFO(g_logger) << "end";

    return 0;
}