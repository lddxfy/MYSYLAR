#ifndef __SYLAR_SINGLETON_H_
#define __SYLAR_SINGLETON_H_
#include <memory>

namespace sylar {

    //X 为了创造多个实例对应的Tag
//N 同一个Tag创造多个实例索引
template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

//X 为了创造多个实例对应的Tag
//N 同一个Tag创造多个实例索引
template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};


}

#endif