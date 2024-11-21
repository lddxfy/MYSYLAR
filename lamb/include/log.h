#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <iostream>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include <unistd.h>
#include <sys/syscall.h>
#include "singleton.h"
#include "util.h"
#include "mutex.h"



#define LOG_LEVEL(logger,level)\
    if(logger->getLevel() <= level) \
        lamb::LogEventWrap(logger,lamb::LogEvent::ptr(new lamb::LogEvent(\
            logger->getName(),level,__FILE__,__LINE__,0,\
            syscall(SYS_gettid),lamb::GetFiberId(),time(0),lamb::GetThreadName()\
        ))).getSS()


#define LOG_FMT_LEVEL(logger,level,fmt,...)\
    if(logger->getLevel() <= level) \
        LogEventWrap(logger,LogEvent::ptr(new LogEvent(\
            logger->getName(),level,__FILE__,__LINE__,0,\
            syscall(SYS_gettid),lamb::GetFiberId(),time(0),lamb::GetThreadName()\
        ))).getEvent()->printf(fmt,__VA_ARGS__)

#define LAMB_LOG_INFO(logger) LOG_LEVEL(logger,lamb::LogLevel::INFO)
#define LAMB_LOG_DEBUG(logger) LOG_LEVEL(logger,lamb::LogLevel::DEBUG)
#define LAMB_LOG_ERROR(logger) LOG_LEVEL(logger,lamb::LogLevel::ERROR)
#define LAMB_LOG_FATAL(logger) LOG_LEVEL(logger,lamb::LogLevel::FATAL)
#define LAMB_LOG_WARN(logger) LOG_LEVEL(logger,lamb::LogLevel::WARN)


#define LAMB_LOG_ROOT() lamb::LoggerMgr::GetInstance()->getRoot()
#define LAMB_LOG_NAME(name) lamb::LoggerMgr::GetInstance()->getLogger(name)

namespace lamb
{

//日志级别
class LogLevel{
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,

        NOTSET = 6 
    };

    static const char* ToString(LogLevel::Level level);

    static LogLevel::Level FromString(const std::string& str);

};

//日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(const std::string&logName,LogLevel::Level level
            , const char* file,int32_t line, uint32_t elapse
            , uint32_t thread_id, uint32_t fiber_id, uint64_t time,const std::string threadName);

    int32_t getLine() const {return m_line;}
    const char* getFile() const {return m_file;}
    uint32_t getElapse() const { return m_elapse;}
    uint32_t getThreadId() const {return m_threadId;}
    uint32_t getFiberId() const {  return m_fiberId;}
    uint64_t getTime() const {return m_time;}
    std::string getContext() const {return m_ss.str();}
    std::stringstream& getSS() {return m_ss;}
    LogLevel::Level getLevel() const {return m_level;}
    std::string getLoggerName() const {return m_loggerName;}
    void setLoggerName(std::string& name) {m_loggerName = name;}
    std::string getThreadName() const {return m_threadName;}

    void printf(const char *fmt,...);
    void vprintf(const char * fmt,va_list ap);
    

private:
    std::string m_loggerName; 
    LogLevel::Level m_level;
    const char* m_file = nullptr;  //文件名
    int32_t m_line = 0;            //行号
    uint32_t m_elapse = 0;         //程序启动到现在的毫秒数
    uint32_t m_threadId = 0;       //线程Id
    uint32_t m_fiberId = 0;        //协程Id
    uint64_t m_time;               //时间戳
    std::string m_threadName;
    std::string m_content; 
    std::stringstream m_ss;    
};


class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
     /**
     * @brief 构造函数
     * @param[in] pattern 格式模板，参考lamb与log4cpp
     * @details 模板参数说明：
     * - %%m 消息
     * - %%p 日志级别
     * - %%c 日志器名称
     * - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
     * - %%r 该日志器创建后的累计运行毫秒数
     * - %%f 文件名
     * - %%l 行号
     * - %%t 线程id
     * - %%F 协程id
     * - %%N 线程名称
     * - %%% 百分号
     * - %%T 制表符
     * - %%n 换行
     * 
     * 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n
     * 
     * 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
     */
    LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l:%m%n");
    //对日志事件进行格式化，返回格式化日志文本
    std::string format(LogEvent::ptr event);
    std::ostream& format(std::ostream &os,LogEvent::ptr event);

    std::string getPattern() const {return m_pattern;}
    //用于初始化，解析格式模板，提取模板项
    void init();
    //模板解析是否出错
    bool isError() const {return m_error;}

public:
    /**
     * @brief 日志内容格式化项，虚基类，用于派生出不同的格式化项
     */
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        virtual ~FormatItem(){}

        virtual void format(std::ostream &os, LogEvent::ptr event) = 0;

    };

private:
    //日志格式模块
    std::string m_pattern;
    //解析后的格式模板数组
    std::vector<FormatItem::ptr> m_items;
    //是否出错
    bool m_error = false;
};

//日志输出地
class LogAppender{
friend class Logger;
public:
    typedef Spinlock MutexType;
    typedef std::shared_ptr<LogAppender> ptr;
    LogAppender(LogFormatter::ptr default_formatter);
    virtual ~LogAppender() {}
    virtual void log(LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val){
        m_formatter = val;
    }
    LogFormatter::ptr getFormatter() const {return m_formatter;}

    void setLevel(LogLevel::Level level){ m_level = level;}
protected:
    MutexType m_mutex;
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
    LogFormatter::ptr m_defaultFormatter;
   
};



//日志输出器
class Logger {
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    
    Logger(const std::string& name = "root");
    void log(LogEvent::ptr event);
    LogLevel::Level getLevel() const {return m_level;}
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    const std::string& getName() const {return m_name;}
    const uint64_t &getCreateTime() const { return m_createTime; }
    void setLevel(LogLevel::Level level){ m_level = level;}
    std::string toYamlString();
private:
    std::list<LogAppender::ptr> m_appenders;//Appender集合
    std::string m_name; //日志名称
    LogLevel::Level m_level;   //日志级别
    uint64_t m_createTime;   //创建时间(毫秒)
    

};
//输出到控制台
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    StdoutLogAppender();
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;
private:

};
//输出到文件
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;
    //重新打开文件，文件打开成功返回true
    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
    bool m_reopenError = false;
};

class LogEventWrap{
public:
    LogEventWrap(Logger::ptr logger,LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event;}
    std::stringstream &getSS();
private:
    Logger::ptr m_logger;
    LogEvent::ptr m_event;
};

class LoggerManager {
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const {return m_root;}
    std::string toYamlString();
private:
    std::map<std::string,Logger::ptr> m_loggers;
    Logger::ptr m_root;
};


typedef lamb::Singleton<LoggerManager> LoggerMgr;

}; // namespace lamb


#endif