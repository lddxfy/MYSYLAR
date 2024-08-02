#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__
#include <string>
#include <iostream>
#include <algorithm>
#include <memory>
#include <functional>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <unordered_set>
#include <sys/stat.h>
#include <unistd.h>

#include "boost/lexical_cast.hpp"
#include "log.h"

namespace sylar{

template<class F,class T>
class LexicalCast {
public:
    T operator()(const F& v){
        return boost::lexical_cast<T>(v);
    }
};
template<class T>
class LexicalCast<std::string,std::vector<T>>{
public:
    std::vector<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0;i<node.size();i++){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::list<T> > {
public:
    std::list<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::set<T> > {
public:
    std::set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
    std::unordered_set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::map<std::string, T> > {
public:
    std::map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};





class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name,const std::string& description = "") : m_name(name),m_description(description){
        std::transform(m_name.begin(),m_name.end(),m_name.begin(),::tolower);
    }
    virtual ~ConfigVarBase(){}

    const std::string& getName() const {return m_name;}
    const std::string& getDescription() const {return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    virtual std::string getTypeName() const = 0;

protected:
    std::string m_name;
    std::string m_description;
};


template<class T,class FromStr = LexicalCast<std::string,T>,class ToStr = LexicalCast<T,std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    ConfigVar(const std::string& name,const T& default_value,const std::string& descripton = "") : ConfigVarBase(name,descripton),m_val(default_value){}

    typedef std::function<void(const T &old_val,const T &new_value)> on_change_cb;
    std::map<uint64_t,on_change_cb> m_cbs;

    uint64_t addListener(on_change_cb cb){
        static uint64_t s_fun_id = 0;
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    void delListener(uint64_t key){
        m_cbs.erase(key);
    }

    on_change_cb getListener(uint64_t key){
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    void clearListener(){
        m_cbs.clear();
    }

    std::string toString() override {
        try{
            //return boost::lexical_cast<std::string>(m_val);
            return ToStr()(m_val);
        }catch(std::exception& e){
            std::cout<<"ConfigVar::toString exception" << e.what() << "convert: " <<typeid(m_val).name()<<"to string";
        }
        return "";
    }
    
    //将字符串类型转为T类型
    bool fromString(const std::string& val) override {
        try{
            //m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        }catch (std::exception& e) {
            std::cout << "ConfigVar::fromString exception"
                << e.what() << " convert: string to " << typeid(m_val).name()
                << " - " << val;
        }
        return false;
    }

    const T getValue() const {return m_val;}
    void setValue(const T& v){
        // 这里有比较运算，需要在自定义类中重载 ==
        if(v == m_val){
            return;
        }
        for(auto &i : m_cbs){
            // 挨个执行回调函数，类似于观察者模式
            i.second(m_val,v);
        }
        m_val = v;
    }
    std::string getTypeName() const override { return typeid(T).name();}
private:
    T m_val;

};

class Config{
public:
    typedef RWMutex RWMutexType;
    typedef std::unordered_map<std::string,ConfigVarBase::ptr> ConfigVarMap;
    //map中有就返回，没有就创建一个并加入到map中
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,const T& default_value,const std::string& description = ""){
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if(tmp){
                MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<<"Lookup name="<< name << " exists";
                return tmp;
            }else {
                MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<<"Lookup name=" << name << " exists but type not "
                        << typeid(T).name() << " real_type=" << it->second->getTypeName()
                        << " " << it->second->toString();
                return nullptr;
            }
        }
        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") != std::string::npos){
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<<"Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
        GetDatas()[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()){
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static void LoadFromYaml(const YAML::Node& root);
    static ConfigVarBase::ptr LookupBase(const std::string& name);
    static void LoadFromConfDir(const std::string &path,bool force = false);
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    static RWMutexType& GetMutex(){
        static RWMutexType s_mutex;
        return s_mutex;
    }
};



}


#endif