#include "include/util.h"
#include "include/log.h"
#include "include/fiber.h"
#include <dirent.h> // 包含 opendir, readdir, closedir 函数的定义
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include <sys/time.h>

namespace lamb{

lamb::Logger::ptr g_logger = LAMB_LOG_NAME("system");

pid_t GetThreadId(){
    return syscall(SYS_gettid);
}

std::string GetThreadName(){
    char thread_name[16] = {0};
    pthread_getname_np(pthread_self(),thread_name,16);
    return std::string(thread_name);
}


uint64_t GetFiberId(){
    return Fiber::GetFiberId();
}

void SetThreadName(const std::string& name){
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
}

void Backtrace(std::vector<std::string>& bt,int size,int skip){
    void** array = (void**)malloc((sizeof(void*)*size));
    size_t s = ::backtrace(array,size);

    char** strings = backtrace_symbols(array,s);
    if(strings == NULL) {
        LAMB_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip;i<s;i++){
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix){
    std::vector<std::string> bt;
    Backtrace(bt,size,skip);
    std::stringstream ss;
    for(size_t i = 0;i < bt.size();++i){
        ss<<prefix<<bt[i]<<std::endl;
    }
    return ss.str();
}
//获取系统启动时到函数执行时的时间间隔
uint64_t GetElapsedMS(){
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t GetCurrentMS(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000*1000ul + tv.tv_usec;
}



std::string ToUpper(const std::string &name){
    std::string rt = name;
    std::transform(rt.begin(),rt.end(),rt.begin(),::toupper);
    return rt;
}

   
std::string ToLower(const std::string &name){
    std::string rt = name;
    std::transform(rt.begin(),rt.end(),rt.begin(),::tolower);
    return rt;
}

  
std::string Time2Str(time_t ts, const std::string &format){
    struct tm tm;
    localtime_r(&ts,&tm);
    char buf[64];
    strftime(buf,sizeof(buf),format.c_str(),&tm);
    return buf;
   
    
}

   
time_t Str2Time(const char *str, const char *format){
    struct tm t;
    memset(&t, 0, sizeof(t));
    if (!strptime(str, format, &t)) {
        return 0;
    }
    return mktime(&t);
}

void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix){
    //检查路径是否存在
    if(access(path.c_str(),0) != 0){
        return;
    }
    //打开目录
    DIR *dir = opendir(path.c_str());
    if(dir == nullptr){
        return;
    }
    struct dirent *dp = nullptr;
    while((dp = readdir(dir)) != nullptr){
        //是一个文件夹
        if(dp->d_type == DT_DIR){
            if(!strcmp(dp->d_name,".") || !strcmp(dp->d_name,"..")){
                continue;
            }
            ListAllFile(files,path + "/" + dp->d_name,subfix);
        }else if(dp->d_type == DT_REG){
            std::string filename(dp->d_name);
            if(subfix.empty()){
                files.push_back(path + "/" +filename);
            }else {
                if(filename.size() < subfix.size()){
                    continue;
                }
                if(filename.substr(filename.length()-subfix.size()) == subfix){
                    files.push_back(path + "/" + filename);
                }
            }
        }
        
    }
    closedir(dir);
}
//用于获取文件或目录的状态信息
static int __lstat(const char *file, struct stat *st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if (st) {
        *st = lst;
    }
    return ret;
}
//用于创建一个新目录
static int __mkdir(const char *dirname){
    if(access(dirname,F_OK) == 0){
        return 0;
    }
    return mkdir(dirname,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}


bool FSUtil::Mkdir(const std::string &dirname){
    //检查文件是否存在
    if(__lstat(dirname.c_str()) == 0){
        return true;
    }
    char *path = strdup(dirname.c_str());
    char *ptr = strchr(path + 1,'/');
    do {
        for(;ptr;*ptr = '/',ptr = strchr(ptr + 1,'/')){
            *ptr = '\0';
            if(__mkdir(path) != 0){
                break;
            }
        }
        if(ptr != nullptr){
            break;
        }else if(__mkdir(path) != 0){
            break;
        }
        free(path);
        return true;
    }while(0);
    free(path);
    return false;

}

        
bool FSUtil::IsRunningPidfile(const std::string &pidfile){
    if(__lstat(pidfile.c_str()) != 0){
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs,line)){
        return false;
    }
    if(line.empty()){
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <=1){
        return false;
    }
    //使用 kill 函数发送信号 0 给指定的 PID。信号 0 不会实际发送信号，但会进行错误检查。
    //如果 kill 返回非零值，表示该 PID 的进程不存在或无法访问，方法返回 false。
    if(kill(pid,0) != 0){
        return false;
    }
    return true;
}

        
bool FSUtil::Rm(const std::string &path){
    struct stat st;
    if(lstat(path.c_str(),&st)){
        return true;
    }
    if(!(st.st_mode & S_IFDIR)){
        return Unlink(path);
    }
    DIR *dir = opendir(path.c_str());
    if(!dir){
        return false;
    }

    bool ret = true;
    struct dirent *dp = nullptr;
    while((dp = readdir(dir))){
        if(!strcmp(dp->d_name,".") || !strcmp(dp->d_name,"..")){
            continue;
        }
        std::string dirname = path + "/" + dp->d_name;
        ret = Rm(dirname);
    }
    closedir(dir);
    if(::rmdir(path.c_str())){
        ret = false;
    }
    return ret;
}

        
bool FSUtil::Mv(const std::string &from, const std::string &to){
    if(!Rm(to)){
        return false;
    }
    return rename(from.c_str(),to.c_str()) == 0;
}

       
bool FSUtil::Realpath(const std::string &path, std::string &rpath){
    if(__lstat(path.c_str())){
        return false;
    }
    char *ptr = ::realpath(path.c_str(),nullptr);
    if(nullptr == ptr){
        return false;
    }
    std::string(ptr).swap(rpath);
    free(ptr);
    return true;
}

        
bool FSUtil::Symlink(const std::string &from, const std::string &to){
    if(!Rm(to)){
        return false;
    }
    return ::symlink(from.c_str(),to.c_str()) ==0;
}

        
bool FSUtil::Unlink(const std::string &filename, bool exist){
    if(!exist && __lstat(filename.c_str())){
        return true;
    }
    return ::unlink(filename.c_str()) == 0;
}

        
std::string FSUtil::Dirname(const std::string &filename){
    if(filename.empty()){
        return ".";
    }
    auto pos = filename.rfind('/');
    if(pos == 0){
        return "/";
    }else if(pos == std::string::npos){
        return ".";
    }else{
        return filename.substr(0,pos);
    }
}

        
std::string FSUtil::Basename(const std::string &filename){
    if(filename.empty()){
        return filename;
    }
    auto pos = filename.rfind('/');
    if(pos == std::string::npos){
        return filename;
    }else{
        return filename.substr(pos + 1);
    }
}

        
bool FSUtil::OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode){
    ifs.open(filename.c_str(),mode);
    return ifs.is_open();
}

        
bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode){
    ofs.open(filename.c_str(),mode);
    if(!ofs.is_open()){
        std::string dir = Dirname(filename);
        Mkdir(dir);
        ofs.open(filename.c_str(),mode);
    }
    return ofs.is_open();
}

int8_t TypeUtil::ToChar(const std::string &str) {
    if (str.empty()) {
        return 0;
    }
    return *str.begin();
}

int64_t TypeUtil::Atoi(const std::string &str) {
    if (str.empty()) {
        return 0;
    }
    return strtoull(str.c_str(), nullptr, 10);
}

double TypeUtil::Atof(const std::string &str) {
    if (str.empty()) {
        return 0;
    }
    return atof(str.c_str());
}

int8_t TypeUtil::ToChar(const char *str) {
    if (str == nullptr) {
        return 0;
    }
    return str[0];
}

int64_t TypeUtil::Atoi(const char *str) {
    if (str == nullptr) {
        return 0;
    }
    return strtoull(str, nullptr, 10);
}

double TypeUtil::Atof(const char *str) {
    if (str == nullptr) {
        return 0;
    }
    return atof(str);
}



       
std::string StringUtil::Format(const char *fmt, ...){
    va_list ap;
    va_start(ap,fmt);
    auto v = Formatv(fmt,ap);
    va_end(ap);
    return v;
}

       
std::string StringUtil::Formatv(const char *fmt, va_list ap){
    char* buf = nullptr;
    auto len = vasprintf(&buf,fmt,ap);
    if(len == -1){
        return "";
    }
    std::string ret(buf,len);
    free(buf);
    return ret;
}

//表示URL中保留字符的编码。数组的每个元素对应一个字符的ASCII值，
//如果该元素为1，则表示该字符是保留字符；如果为0，则表示该字符不是保留字符。
static const char uri_chars[256] = {
    /* 0 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 0, 0, 0, 1, 0, 0,
    /* 64 */
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,
    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    /* 192 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};
//表示十六进制字符的编码。数组的每个元素对应一个十六进制字符的ASCII值，
//如果该元素为1，则表示该字符是十六进制字符；如果为0，则表示该字符不是十六进制字符。
static const char xdigit_chars[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
//用于检查一个字符是否为保留字符
#define CHAR_IS_UNRESERVED(c)           \
    (uri_chars[(unsigned char)(c)])

//将输入字符串进行URL编码
//-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~
std::string StringUtil::UrlEncode(const std::string& str, bool space_as_plus) {
    static const char *hexdigits = "0123456789ABCDEF";
    std::string* ss = nullptr;
    const char* end = str.c_str() + str.length();
    for(const char* c = str.c_str() ; c < end; ++c) {
        if(!CHAR_IS_UNRESERVED(*c)) {
            if(!ss) {
                ss = new std::string;
                ss->reserve(str.size() * 1.2);
                ss->append(str.c_str(), c - str.c_str());
            }
            if(*c == ' ' && space_as_plus) {
                ss->append(1, '+');
            } else {
                ss->append(1, '%');
                ss->append(1, hexdigits[(uint8_t)*c >> 4]);
                ss->append(1, hexdigits[*c & 0xf]);
            }
        } else if(ss) {
            ss->append(1, *c);
        }
    }
    if(!ss) {
        return str;
    } else {
        std::string rt = *ss;
        delete ss;
        return rt;
    }
}


        
std::string StringUtil::UrlDecode(const std::string& str, bool space_as_plus) {
    std::string* ss = nullptr;
    const char* end = str.c_str() + str.length();
    for(const char* c = str.c_str(); c < end; ++c) {
        if(*c == '+' && space_as_plus) {
            if(!ss) {
                ss = new std::string;
                ss->append(str.c_str(), c - str.c_str());
            }
            ss->append(1, ' ');
        } else if(*c == '%' && (c + 2) < end
                    && isxdigit(*(c + 1)) && isxdigit(*(c + 2))){
            if(!ss) {
                ss = new std::string;
                ss->append(str.c_str(), c - str.c_str());
            }
            ss->append(1, (char)(xdigit_chars[(int)*(c + 1)] << 4 | xdigit_chars[(int)*(c + 2)]));
            c += 2;
        } else if(ss) {
            ss->append(1, *c);
        }
    }
    if(!ss) {
        return str;
    } else {
        std::string rt = *ss;
        delete ss;
        return rt;
    }
}

        
std::string StringUtil::Trim(const std::string &str, const std::string &delimit){
    auto begin = str.find_first_not_of(delimit);
    if(begin == std::string::npos){
        return "";
    }
    auto end = str.find_last_not_of(delimit);
    return str.substr(begin,end-begin+1);
}

        
std::string StringUtil::TrimLeft(const std::string &str, const std::string &delimit){
    auto begin = str.find_first_not_of(delimit);
    if(begin == std::string::npos){
        return "";
    }
    return str.substr(begin);
}

std::string StringUtil::TrimRight(const std::string &str, const std::string &delimit){
    auto end = str.find_last_not_of(delimit);
    if(end == std::string::npos) {
        return "";
    }
    return str.substr(0, end);
}

       
std::string StringUtil::WStringToString(const std::wstring &ws){
    std::string str_locale = setlocale(LC_ALL, "");
    const wchar_t* wch_src = ws.c_str();
    size_t n_dest_size = wcstombs(NULL, wch_src, 0) + 1;
    char *ch_dest = new char[n_dest_size];
    memset(ch_dest,0,n_dest_size);
    wcstombs(ch_dest,wch_src,n_dest_size);
    std::string str_result = ch_dest;
    delete []ch_dest;
    setlocale(LC_ALL, str_locale.c_str());
    return str_result;
}

std::wstring StringUtil::StringToWString(const std::string &s){
    std::string str_locale = setlocale(LC_ALL, "");
    const char* chSrc = s.c_str();
    size_t n_dest_size = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t* wch_dest = new wchar_t[n_dest_size];
    wmemset(wch_dest, 0, n_dest_size);
    mbstowcs(wch_dest,chSrc,n_dest_size);
    std::wstring wstr_result = wch_dest;
    delete []wch_dest;
    setlocale(LC_ALL, str_locale.c_str());
    return wstr_result;
}

}