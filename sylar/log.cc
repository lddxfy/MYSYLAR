#include "log.h"
#include "config.h"
#include <map>
#include <functional>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
namespace sylar {

const char* LogLevel::ToString(LogLevel::Level level){
    switch (level)
    {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;
    
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
}

LogLevel::Level LogLevel::FromString(const std::string& str){
#define XX(level,v) \
    if(str == #v){  \
        return LogLevel::level; \
    }
    XX(DEBUG,debug);
    XX(INFO,info);
    XX(WARN,warn);
    XX(ERROR,error);
    XX(FATAL,fatal);
    
    XX(DEBUG,DEBUG);
    XX(INFO,INFO);
    XX(WARN,WARN);
    XX(ERROR,ERROR);
    XX(FATAL,FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

LogEvent::LogEvent(const std::string&logName,LogLevel::Level level
            , const char* file,int32_t line, uint32_t elapse
            , uint32_t thread_id, uint32_t fiber_id, uint64_t time,const std::string threadName)
            :m_loggerName(logName)
            ,m_level(level)
            ,m_file(file)
            ,m_line(line)
            ,m_elapse(elapse)
            ,m_threadId(thread_id)
            ,m_fiberId(fiber_id)
            ,m_time(time)
            ,m_threadName(threadName)
            
{

}

void LogEvent::printf(const char *fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
}

void LogEvent::vprintf(const char *fmt, va_list ap){
    char *buf = nullptr;
    int len = vasprintf(&buf,fmt,ap);
    if(len != -1) {
        m_ss << std::string(buf,len);
        free(buf);
    }
}

            

Logger::Logger(const std::string& name):m_name(name),m_level(LogLevel::DEBUG){
    
}


void Logger::addAppender(LogAppender::ptr appender){
    m_appenders.push_back(appender);

    
}
void Logger::delAppender(LogAppender::ptr appender){
    for(auto it = m_appenders.begin();it!=m_appenders.end();it++){
        if(*it ==appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders(){
    m_appenders.clear();
}



std::string Logger::toYamlString(){
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::ToString(m_level);
    }
    
    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void Logger::log(LogEvent::ptr event){
    if(event->getLevel() >= m_level){
        for(auto& i : m_appenders){
            i->log(event);
        }
    }
}

// void Logger::debug(LogEvent::ptr event){
//     log(LogLevel::DEBUG,event);
// }
// void Logger::info(LogEvent::ptr event){
//     log(LogLevel::INFO,event);
// }
// void Logger::warn(LogEvent::ptr event){
//     log(LogLevel::WARN,event);
// }
// void Logger::error(LogEvent::ptr event){
//     log(LogLevel::ERROR,event);
// }
// void Logger::fatal(LogEvent::ptr event){
//     log(LogLevel::FATAL,event);
// }

LogAppender::LogAppender(LogFormatter::ptr default_formatter) : m_defaultFormatter(default_formatter){

}


FileLogAppender::FileLogAppender(const std::string& filename) : LogAppender(LogFormatter::ptr(new LogFormatter)),m_filename(filename){
    reopen();
    if(m_reopenError){
        std::cout<<"reopen file" << m_filename << " error" << std::endl;
    }
}


std::string FileLogAppender::toYamlString(){
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    node["pattern"] = m_formatter ? m_formatter->getPattern() : m_defaultFormatter->getPattern();
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
/**
 * 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
 */
void FileLogAppender::log(LogEvent::ptr event)  {
    uint64_t now = event->getTime();
    if(now >= (m_lastTime + 3)){
        reopen();
        if(m_reopenError){
            std::cout<< "reopen file" << m_filename << " error" <<std::endl;
        }
        m_lastTime = now;
    }
    if(m_reopenError){
        return;
    }
    MutexType::Lock lock(m_mutex);
    if(m_formatter) {
        if(!m_formatter->format(m_filestream, event)) {
            std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        }
    } else {
        if(!m_defaultFormatter->format(m_filestream, event)) {
            std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        }
    }
}

bool FileLogAppender::reopen(){
    MutexType::Lock lock(m_mutex);
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename,std::ios::app);
    m_reopenError = !m_filestream;
    return !m_reopenError;
}

StdoutLogAppender::StdoutLogAppender() : LogAppender(LogFormatter::ptr(new LogFormatter)){}


std::string StdoutLogAppender::toYamlString(){
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["pattern"] = m_formatter ? m_formatter->getPattern() : m_defaultFormatter->getPattern();
    std::stringstream ss;
    ss << node;
    return ss.str();
}


void StdoutLogAppender::log(LogEvent::ptr event)  {
    if(event->getLevel() >= m_level){
        if(m_formatter) {
            std::cout<< m_formatter->format(event);
        }else {
            std::cout<<m_defaultFormatter->format(event);
        }
    }
}



LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern){
    init();
}

std::string LogFormatter::format(LogEvent::ptr event){
    std::stringstream ss;
    for(auto& i : m_items){
        i->format(ss,event);
    }
    return ss.str();
}

std::ostream &LogFormatter::format(std::ostream &os,LogEvent::ptr event){
    for(auto &i : m_items){
        i->format(os,event);
    }
    return os;
}

class MessageFormatter : public LogFormatter::FormatItem{
public:
    MessageFormatter(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getContext();
    }


private:
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<LogLevel::ToString(event->getLevel());
    }
private:

};

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getElapse();
    }
private:

};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getThreadId();
    }
private:

};

class FiberIdFormatItem : public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& str){}  
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getFiberId();
    }
private:

};

class DateTimeFormatItem : public LogFormatter::FormatItem{
public:
    DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S") : m_format(format){
        if(m_format.empty()){
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream &os, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time,&tm);
        char buf[64];
        strftime(buf,sizeof(buf),m_format.c_str(),&tm);
        os<<buf;
        
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem{
public:
    FilenameFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getFile();
    }
private:  
};

class LineFormatItem : public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getLine();
    }
private:  
};

class LoggerNameFormatItem : public LogFormatter::FormatItem{
public:
    LoggerNameFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<event->getLoggerName();
    }
private:  
};


class NewLineFormatItem : public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<std::endl;
    }
private:  
};

class TabFormatItem : public LogFormatter::FormatItem{
public:
    TabFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os<<"\t";
    }
private:  
};

class PercentSignFormatItem : public LogFormatter::FormatItem {
public:
    PercentSignFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << "%";
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};


class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
private:
    
};

/**
 * 简单的状态机判断，提取pattern中的常规字符和模式字符
 * 
 * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
 * 
 * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
 * 
 * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这个状态需要特殊处理
 * 
 * 一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
 * 
 * @see LogFormatter::LogFormatter
 */
void LogFormatter::init(){
    // 按顺序存储解析到的pattern项
    // 每个pattern包括一个整数类型和一个字符串，类型为0表示该pattern是常规字符串，为1表示该pattern需要转义
    // 日期格式单独用下面的dataformat存储
    std::vector<std::pair<int,std::string>> patterns;
    //临时存储常规字符串
    std::string tmp;
    //日期格式字符串，默认把位于%d后面的大括号对里的全部字符都当作格式字符，不校验格式是否合法
    std::string dateformat;
    //是否解析出错
    bool error = false;

    //是否正在解析常规字符，初始时为true
    bool parsing_string = true;

    size_t i = 0;
    while (i< m_pattern.size())
    {
        std::string c = std::string(1,m_pattern[i]);
        if(c == "%"){
            if(parsing_string){
                if(!tmp.empty()){
                    patterns.push_back(std::make_pair(0,tmp));
                }
                tmp.clear();
                parsing_string = false;
                i++;
                continue;
            }else{
                patterns.push_back(std::make_pair(1,c));
                parsing_string = true;// 在解析模板字符时遇到%，表示这里是一个%转义
                i++;
                continue;
            }
        }else{
            if(parsing_string){// 持续解析常规字符直到遇到%，解析出的字符串作为一个常规字符串加入patterns
                tmp += c;
                i++;
                continue;
            }else{ // 模板字符，直接添加到patterns中，添加完成后，状态变为解析常规字符，%d特殊处理
                patterns.push_back(std::make_pair(1,c));
                parsing_string = true;

                if(c!="d"){
                    i++;
                    continue;
                }
                i++;
                if(i<m_pattern.size() && m_pattern[i]!='{'){
                    continue;
                }
                i++;
                while(i<m_pattern.size() && m_pattern[i] != '}'){
                    dateformat.push_back(m_pattern[i]);
                    i++;
                }
                if(m_pattern[i]!='}'){
                    // %d后面的大括号没有闭合，直接报错
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] '{' not closed" << std::endl;
                    error = true;
                    break;
                }
                i++;
                continue;
            }
        }
    }
    if(error){
        m_error = true;
        return;
    }

    // 模板解析结束之后剩余的常规字符也要算进去
    if(!tmp.empty()){
        patterns.push_back(std::make_pair(0,tmp));
        tmp.clear();
    }

    static std::map<std::string,std::function<FormatItem::ptr(const std::string& str)>> s_format_items={
#define XX(str,C) {#str,[](const std::string& fmt) {return FormatItem::ptr(new C(fmt));}}

        XX(m,MessageFormatter),         // m:消息
        XX(p,LevelFormatItem),          // p:日志级别
        XX(c,LoggerNameFormatItem),     // c:日志器名称
        XX(r,ElapseFormatItem),         // r:累计毫秒数
        XX(f,FilenameFormatItem),       // f:文件名
        XX(l,LineFormatItem),           // l:行号
        XX(t,ThreadIdFormatItem),       // t:线程号
        XX(F,FiberIdFormatItem),        // F:协程号
        XX(N,ThreadNameFormatItem),
        XX(n,NewLineFormatItem),        // n:换行符
        XX(T,TabFormatItem),            // T:制表符
        XX(%,PercentSignFormatItem),    // %:百分号
#undef XX
    };

    for(auto &v : patterns){
        if(v.first == 0){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
        }else if(v.second == "d"){
            m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
        }else {
            auto it = s_format_items.find(v.second);
            if(it == s_format_items.end()){
                std::cout<< "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] " << 
                "unknown format item: " << v.second << std::endl;
                error = true;
                break;
            }else {
                m_items.push_back(it->second(v.second));
            }
        }
    }

    if(error) {
        m_error = true;
        return;
    }

};

LogEventWrap::LogEventWrap(Logger::ptr logger,LogEvent::ptr e) : m_logger(logger),m_event(e)
{}

LogEventWrap::~LogEventWrap(){
    m_logger->log(m_event);
}

std::stringstream& LogEventWrap::getSS(){
    return m_event->getSS();
}


LoggerManager::LoggerManager(){
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->getName()] = m_root;
}

Logger::ptr LoggerManager::getLogger(const std::string& name){
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()){
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString(){
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

struct LogAppendDefine 
{
    int type = 0; //1 File,2 Stdout
    std::string pattern;
    std::string file;

    bool operator==(const LogAppendDefine& oth) const {
        return type == oth.type
            && pattern == oth.pattern
            && file == oth.file;
    }
};


struct LogDefine
{
    std::string name;
    LogLevel::Level level = LogLevel::NOTSET;
    std::vector<LogAppendDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

template<>
class LexicalCast<std::string,LogDefine> {
public:
    LogDefine operator()(const std::string& v){
        // 将文本格式YAML数据v解析为内存中YAML节点n
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()){
            std::cout<<"log config error: name is null,"<< n <<std::endl;
            throw std::logic_error("log config name is null");
        }
        // 将n["name"]值转为string赋给ld.name
        ld.name = n["name"].as<std::string>();
        // 若n定义了level，则直接赋值，否则为空
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>():"");
        

        // 若n定义了appenders
        if(n["appenders"].IsDefined()){
            for(size_t x = 0; x<n["appenders"].size();++x){
                auto a = n["appenders"][x];
                // 若当前appender没有定义type，输出错误日志
                if(!a["type"].IsDefined()){
                    std::cout << "log config error: appender type is null, " << a
                                << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                // 定义LogAppenderDefine对象 lad
                LogAppendDefine lad;
                if(type == "FileLogAppender"){
                    lad.type = 1;
                    if(!a["file"].IsDefined()){
                        std::cout << "log config error: fileappender file is null, " << a
                                << std::endl;
                        continue;
                    }
                    // 设置文件路径
                    lad.file = a["file"].as<std::string>();
                    // 设置appender的formatter
                    if(a["pattern"].IsDefined()){
                        lad.pattern = a["pattern"].as<std::string>();
                    }
                }else if(type == "StdoutLogAppender"){
                    lad.type = 2;
                    if(a["pattern"].IsDefined()){
                        lad.pattern = a["pattern"].as<std::string>();
                    }
                }else {
                    std::cout << "log config error: appender type is invalid, " << a
                                << std::endl;
                    continue;
                }
                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }

};

template<>
class LexicalCast<LogDefine,std::string> {
public:
    std::string operator()(const LogDefine& i){
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKNOW){
            n["level"] = LogLevel::ToString(i.level);
        }

        for(auto& a : i.appenders){
            YAML::Node na;
            if(a.type == 1){
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            }else if(a.type == 2){
                na["type"] = "StdoutLogAppender";
            }
            if(!a.pattern.empty()){
                na["pattern"] = a.pattern;
            }

            n["appender"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines = 
    sylar::Config::Lookup("logs",std::set<LogDefine>(),"logs config");

struct LogIniter {
    LogIniter() {
        // 添加变化回调函数
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,const std::set<LogDefine>& new_value){
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto& i : new_value){
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                // new有 old没有
                if(it == old_value.end()){
                    logger = MYSYLAR_LOG_NAME(i.name);
                }
                else{
                    if(!(i == *it)){
                        // 修改的logger
                        logger = MYSYLAR_LOG_NAME(i.name);
                    }else {
                        continue;
                    }
                }
                // 设置level
                logger->setLevel(i.level);
                logger->clearAppenders();
                // 设置appenders
                for(auto &a : i.appenders){
                    sylar::LogAppender::ptr ap;
                    //File
                    if(a.type == 1){
                        ap.reset(new FileLogAppender(a.file));
                    }//stdout
                    else if(a.type == 2){
                        ap.reset(new StdoutLogAppender);
                    }

                    //设置appender的formatter
                    if(!a.pattern.empty()){
                        ap->setFormatter(LogFormatter::ptr(new LogFormatter(a.pattern)));
                    }else {
                        ap->setFormatter(LogFormatter::ptr(new LogFormatter));
                    }
                    logger->addAppender(ap);
                }
            }
            for(auto& i : old_value){
                auto it = new_value.find(i);
                // old有 new没有
                if(it==new_value.end()){
                    //删除logger
                    auto logger = MYSYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppenders();
                }
            }

        });
    }
};


// 保证在main之前初始化
static LogIniter __log_init;

}




