#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "../include/config.h"
#include "../include/log.h"
#include "../include/env.h"

#include <unistd.h>

sylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT(); // 默认INFO级别

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    MYSYLAR_LOG_FATAL(g_logger) << "fatal msg";
    MYSYLAR_LOG_ERROR(g_logger) << "err msg";
    MYSYLAR_LOG_INFO(g_logger) << "info msg";
    MYSYLAR_LOG_DEBUG(g_logger) << "debug msg";

    //MYSYLAR_LOG_FMT_FATAL(g_logger, "fatal %s:%d", __FILE__, __LINE__);
    //MYSYLAR_LOG_FMT_ERROR(g_logger, "err %s:%d", __FILE__, __LINE__);
    //MYSYLAR_LOG_FMT_INFO(g_logger, "info %s:%d", __FILE__, __LINE__);
    //MYSYLAR_LOG_FMT_DEBUG(g_logger, "debug %s:%d", __FILE__, __LINE__);
   
    sleep(1);
    sylar::SetThreadName("brand_new_thread");

    g_logger->setLevel(sylar::LogLevel::WARN);
    MYSYLAR_LOG_FATAL(g_logger) << "fatal msg";
    MYSYLAR_LOG_ERROR(g_logger) << "err msg";
    MYSYLAR_LOG_INFO(g_logger) << "info msg"; // 不打印
    MYSYLAR_LOG_DEBUG(g_logger) << "debug msg"; // 不打印


    sylar::FileLogAppender::ptr fileAppender(new sylar::FileLogAppender("./log.txt"));
    g_logger->addAppender(fileAppender);
    MYSYLAR_LOG_FATAL(g_logger) << "fatal msg";
    MYSYLAR_LOG_ERROR(g_logger) << "err msg";
    MYSYLAR_LOG_INFO(g_logger) << "info msg"; // 不打印
    MYSYLAR_LOG_DEBUG(g_logger) << "debug msg"; // 不打印

    sylar::Logger::ptr test_logger = MYSYLAR_LOG_NAME("test_logger");
    sylar::StdoutLogAppender::ptr appender(new sylar::StdoutLogAppender);
    sylar::LogFormatter::ptr formatter(new sylar::LogFormatter("%d:%rms%T%p%T%c%T%f:%l %m%n")); // 时间：启动毫秒数 级别 日志名称 文件名：行号 消息 换行
    appender->setFormatter(formatter);
    test_logger->addAppender(appender);
    test_logger->setLevel(sylar::LogLevel::WARN);

    MYSYLAR_LOG_ERROR(test_logger) << "err msg";
    MYSYLAR_LOG_INFO(test_logger) << "info msg"; // 不打印

    //sylar::Logger::ptr test_logger2 = MYSYLAR_LOG_NAME("system");
    //MYSYLAR_LOG_DEBUG(test_logger2) << "hello world";


    // 输出全部日志器的配置
    g_logger->setLevel(sylar::LogLevel::INFO);
    MYSYLAR_LOG_INFO(g_logger) << "logger config:" << sylar::LoggerMgr::GetInstance()->toYamlString();

    return 0;
}