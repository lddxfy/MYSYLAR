#include <iostream>
#include <pthread.h>
#include "thread.h"
#include "log.h"

sylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

int count = 0;
sylar::Mutex s_mutex;

void printFunc(void* arg){
    MYSYLAR_LOG_INFO(g_logger) << "name:" << sylar::Thread::GetName()
            << " this.name: " << sylar::Thread::GetThis()->GetName()
            << " thread name:" << sylar::GetThreadName()
            << " id:" << sylar::GetThreadId()
            << " this.id" <<sylar::Thread::GetThis()->getId();
    MYSYLAR_LOG_INFO(g_logger) << " arg: " <<*(int*) arg;
    for(int i = 0;i < 10000; i++){
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

int main(){

    std::vector<sylar::Thread::ptr> thrs;
    int arg = 123456;
    
    for(int i = 0;i<3;i++){
        sylar::Thread::ptr thr(new sylar::Thread(std::bind(printFunc,&arg),"thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 3; i++) {
        thrs[i]->join();
    }
    
    MYSYLAR_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}