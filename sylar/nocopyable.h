#ifndef __SYLAR_NOCOPYABLE_H__
#define __SYLAR_NOCOPYABLE_H__

namespace sylar{

class Noncopyable{
public:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable(const Noncopyable&&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
};


}


#endif