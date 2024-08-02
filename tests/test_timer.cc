/**
 * @file test_timer.cc
 * @brief IO协程测试器定时器测试
 * @version 0.1
 * @date 2021-06-19
 */

#include "iomanager.h"
#include "log.h"
#include "config.h"
#include "env.h"

static sylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

static int timeout = 1000;
static sylar::Timer::ptr s_timer;

struct timer_info {
    int cancelled = 0;
};

void timer_callback() {
    MYSYLAR_LOG_INFO(g_logger) << "timer callback, timeout = " << timeout;
    timeout += 1000;
    if(timeout < 5000) {
        s_timer->reset(timeout, true);
    } else {
        s_timer->cancel();
    }
}

void test_timer() {
    sylar::IOManager iom;

    // 循环定时器
    // s_timer = iom.addTimer(1000, timer_callback, true);
    
    // 单次定时器
    auto timer = iom.addTimer(1000, []{
        MYSYLAR_LOG_INFO(g_logger) << "500ms timeout";
    });
    // std::shared_ptr<timer_info> tinfo(new timer_info);
    // std::weak_ptr<timer_info> winfo(tinfo);
    // iom.addConditionTimer(1000,[](){
    //     MYSYLAR_LOG_INFO(g_logger) << "1000ms timeout";

    // },winfo);

    
    // iom.addTimer(5000, []{
    //     MYSYLAR_LOG_INFO(g_logger) << "5000ms timeout";
    // });
    timer->cancel();
    
}

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    test_timer();

    MYSYLAR_LOG_INFO(g_logger) << "end";

    return 0;
}