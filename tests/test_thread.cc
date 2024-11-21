#include <iostream>
#include <pthread.h>
#include "../include/thread.h"
#include "../include/log.h"

lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

int count = 0;
lamb::Mutex s_mutex;

void printFunc(void* arg){
    LAMB_LOG_INFO(g_logger) << "name:" << lamb::Thread::GetName()
            << " this.name: " << lamb::Thread::GetThis()->GetName()
            << " thread name:" << lamb::GetThreadName()
            << " id:" << lamb::GetThreadId()
            << " this.id" <<lamb::Thread::GetThis()->getId();
    LAMB_LOG_INFO(g_logger) << " arg: " <<*(int*) arg;
    for(int i = 0;i < 10000; i++){
        lamb::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

int main(){

    std::vector<lamb::Thread::ptr> thrs;
    int arg = 123456;
    
    for(int i = 0;i<3;i++){
        lamb::Thread::ptr thr(new lamb::Thread(std::bind(printFunc,&arg),"thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 3; i++) {
        thrs[i]->join();
    }
    
    LAMB_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}