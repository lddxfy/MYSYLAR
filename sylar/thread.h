#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__
#include <pthread.h>
#include <functional>
#include <semaphore.h>
#include <memory>
#include <sys/types.h>
#include <sys/syscall.h>
#include "nocopyable.h"
#include "mutex.h"
#include "util.h"
namespace sylar{

class Thread : public Noncopyable{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb,const std::string& name);
    ~Thread();

    pid_t getId() const {
        return m_id;
    }

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    static void* run(void* arg);

    pthread_t m_thread = 0;
    pid_t m_id;
    std::function<void()> m_cb;
    std::string m_name;
    Semaphore m_semaphore;

};

}
#endif