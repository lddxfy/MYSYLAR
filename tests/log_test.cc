#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "../include/config.h"
#include "../include/log.h"
#include "../include/env.h"

#include <unistd.h>

lamb::Logger::ptr g_logger = LAMB_LOG_ROOT(); // 默认INFO级别

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    LAMB_LOG_FATAL(g_logger) << "fatal msg";
    LAMB_LOG_ERROR(g_logger) << "err msg";
    LAMB_LOG_INFO(g_logger) << "info msg";
    LAMB_LOG_DEBUG(g_logger) << "debug msg";

    //LAMB_LOG_FMT_FATAL(g_logger, "fatal %s:%d", __FILE__, __LINE__);
    //LAMB_LOG_FMT_ERROR(g_logger, "err %s:%d", __FILE__, __LINE__);
    //LAMB_LOG_FMT_INFO(g_logger, "info %s:%d", __FILE__, __LINE__);
    //LAMB_LOG_FMT_DEBUG(g_logger, "debug %s:%d", __FILE__, __LINE__);
   
    sleep(1);
    lamb::SetThreadName("brand_new_thread");

    g_logger->setLevel(lamb::LogLevel::WARN);
    LAMB_LOG_FATAL(g_logger) << "fatal msg";
    LAMB_LOG_ERROR(g_logger) << "err msg";
    LAMB_LOG_INFO(g_logger) << "info msg"; // 不打印
    LAMB_LOG_DEBUG(g_logger) << "debug msg"; // 不打印


    lamb::FileLogAppender::ptr fileAppender(new lamb::FileLogAppender("./log.txt"));
    g_logger->addAppender(fileAppender);
    LAMB_LOG_FATAL(g_logger) << "fatal msg";
    LAMB_LOG_ERROR(g_logger) << "err msg";
    LAMB_LOG_INFO(g_logger) << "info msg"; // 不打印
    LAMB_LOG_DEBUG(g_logger) << "debug msg"; // 不打印

    lamb::Logger::ptr test_logger = LAMB_LOG_NAME("test_logger");
    lamb::StdoutLogAppender::ptr appender(new lamb::StdoutLogAppender);
    lamb::LogFormatter::ptr formatter(new lamb::LogFormatter("%d:%rms%T%p%T%c%T%f:%l %m%n")); // 时间：启动毫秒数 级别 日志名称 文件名：行号 消息 换行
    appender->setFormatter(formatter);
    test_logger->addAppender(appender);
    test_logger->setLevel(lamb::LogLevel::WARN);

    LAMB_LOG_ERROR(test_logger) << "err msg";
    LAMB_LOG_INFO(test_logger) << "info msg"; // 不打印

    //lamb::Logger::ptr test_logger2 = LAMB_LOG_NAME("system");
    //LAMB_LOG_DEBUG(test_logger2) << "hello world";


    // 输出全部日志器的配置
    g_logger->setLevel(lamb::LogLevel::INFO);
    LAMB_LOG_INFO(g_logger) << "logger config:" << lamb::LoggerMgr::GetInstance()->toYamlString();

    return 0;
}