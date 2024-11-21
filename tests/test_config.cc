#include<iostream>
#include<yaml-cpp/yaml.h>
#include "../include/config.h"
#include "../include/env.h"
#include "../include/log.h"



lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

lamb::ConfigVar<int>::ptr g_int = 
    lamb::Config::Lookup("global.int", (int)8080, "global int");

lamb::ConfigVar<float>::ptr g_float = 
    lamb::Config::Lookup("global.float", (float)10.2f, "global float");

// 字符串需显示构造，不能传字符串常量
lamb::ConfigVar<std::string>::ptr g_string =
    lamb::Config::Lookup("global.string", std::string("helloworld"), "global string");

lamb::ConfigVar<std::vector<int>>::ptr g_int_vec = 
    lamb::Config::Lookup("global.int_vec", std::vector<int>{1, 2, 3}, "global int vec");

lamb::ConfigVar<std::list<int>>::ptr g_int_list = 
    lamb::Config::Lookup("global.int_list", std::list<int>{1, 2, 3}, "global int list");

lamb::ConfigVar<std::set<int>>::ptr g_int_set = 
    lamb::Config::Lookup("global.int_set", std::set<int>{1, 2, 3}, "global int set");

lamb::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set = 
    lamb::Config::Lookup("global.int_unordered_set", std::unordered_set<int>{1, 2, 3}, "global int unordered_set");

lamb::ConfigVar<std::map<std::string, int>>::ptr g_map_string2int = 
    lamb::Config::Lookup("global.map_string2int", std::map<std::string, int>{{"key1", 1}, {"key2", 2}}, "global map string2int");

lamb::ConfigVar<std::unordered_map<std::string, int>>::ptr g_unordered_map_string2int = 
    lamb::Config::Lookup("global.unordered_map_string2int", std::unordered_map<std::string, int>{{"key1", 1}, {"key2", 2}}, "global unordered_map string2int");

class Person{
public:
    std::string name = "";
    int age = 0;
    bool sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << name
           << "age=" << age
           << "sex=" << sex <<"]";
        return ss.str();
    }

    bool operator==(const Person& val) const{
        if(this->name == val.name && this->age==val.age && this->sex==val.sex){
            return true;
        }
        return false;
    }
};

// 实现自定义配置的YAML序列化与反序列化，这部分要放在lamb命名空间中
namespace lamb {

template<>
class LexicalCast<std::string,Person> {
public:
    Person operator()(const std::string &v) const {
        std::cout<<1<<std::endl;
        YAML::Node node = YAML::Load(v);
        Person p;
       // std::cout<<node["name"].as<std::string>()<<std::endl;
        p.name = node["name"].as<std::string>();
        p.age = node["age"].as<int>();
        p.sex = node["sex"].as<bool>();
        
        return p;
    }
};

template<>
class LexicalCast<Person,std::string> {
public:
    std::string operator()(const Person &v) const {
        YAML::Node node;
       
        node["name"]=v.name;
        node["age"]=v.age;
        node["sex"]=v.sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}


lamb::ConfigVar<Person>::ptr g_person = 
    lamb::Config::Lookup("class.person", Person(), "system person");

lamb::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = 
    lamb::Config::Lookup("class.map", std::map<std::string, Person>(), "system person map");

lamb::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map = 
    lamb::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system vec map");



static void ListAllMember(const std::string& prefix,const YAML::Node& node,std::list<std::pair<std::string,const YAML::Node>>& output){
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")!=std::string::npos){
        LAMB_LOG_ERROR(LAMB_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix,node));
    if(node.IsMap()){
        for(auto it = node.begin();it!=node.end();it++){
            ListAllMember(prefix.empty()?it->first.Scalar() : prefix+"."+it->first.Scalar(),it->second,output);
        }
    }
}

void test_class() {
    static uint64_t id = 0;

    if(!g_person->getListener(id)) {
        id = g_person->addListener([](const Person &old_value, const Person &new_value){
            LAMB_LOG_INFO(g_logger) << "g_person value change, old value:" << old_value.toString()
                << ", new value:" << new_value.toString();
        });
    }

    LAMB_LOG_INFO(g_logger) << g_person->getValue().toString();

    for (const auto &i : g_person_map->getValue()) {
        LAMB_LOG_INFO(g_logger) << i.first << ":" << i.second.toString();
    }

    for(const auto &i : g_person_vec_map->getValue()) {
        LAMB_LOG_INFO(g_logger) << i.first;
        for(const auto &j : i.second) {
            LAMB_LOG_INFO(g_logger) << j.toString();
        }
    }
}

template<class T>
std::string formatArray(const T &v) {
    std::stringstream ss;
    ss << "[";
    for(const auto &i:v) {
        ss << " " << i;
    }
    ss << " ]";
    return ss.str();
}

template<class T>
std::string formatMap(const T &m) {
    std::stringstream ss;
    ss << "{";
    for(const auto &i:m) {
        ss << " {" << i.first << ":" << i.second << "}";
    }
    ss << " }";
    return ss.str();
}

void test_config() {
    LAMB_LOG_INFO(g_logger) << "g_int value: " << g_int->getValue();
    LAMB_LOG_INFO(g_logger) << "g_float value: " << g_float->getValue();
    LAMB_LOG_INFO(g_logger) << "g_string value: " << g_string->getValue();
    LAMB_LOG_INFO(g_logger) << "g_int_vec value: " << formatArray<std::vector<int>>(g_int_vec->getValue());
    LAMB_LOG_INFO(g_logger) << "g_int_list value: " << formatArray<std::list<int>>(g_int_list->getValue());
    LAMB_LOG_INFO(g_logger) << "g_int_set value: " << formatArray<std::set<int>>(g_int_set->getValue());
    LAMB_LOG_INFO(g_logger) << "g_int_unordered_set value: " << formatArray<std::unordered_set<int>>(g_int_unordered_set->getValue());
    LAMB_LOG_INFO(g_logger) << "g_int_map value: " << formatMap<std::map<std::string, int>>(g_map_string2int->getValue());
    LAMB_LOG_INFO(g_logger) << "g_int_unordered_map value: " << formatMap<std::unordered_map<std::string, int>>(g_unordered_map_string2int->getValue());

    // 自定义配置项
    test_class();
}

void test_log(){
    static lamb::Logger::ptr system_log = LAMB_LOG_NAME("system");
    LAMB_LOG_INFO(system_log) << "hello system" << std::endl;

    std::cout << lamb::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    // YAML::Node root = YAML::LoadFile("/home/chensir/Documents/mylamb/bin/log.yaml");
    //lamb::Config::LoadFromYaml(root);

   

    std::cout << "=============" << std::endl;
    std::cout << lamb::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    //std::cout << root << std::endl;
    LAMB_LOG_INFO(system_log) << "hello system" << std::endl;
}




int main(int argc,char *argv[]){

    //test_log();
    
    g_int->addListener([](const int &old_value, const int &new_value) {
        LAMB_LOG_INFO(g_logger) << "g_int value changed, old_value: " << old_value << ", new_value: " << new_value;
    });

    LAMB_LOG_INFO(g_logger) << "before============================";
    
    test_config();
    // 从配置文件中加载配置，由于更新了配置，会触发配置项的配置变更回调函数

    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir("conf");
    
    LAMB_LOG_INFO(g_logger) << "after============================";
    
    test_config();

    // 遍历所有配置
    lamb::Config::Visit([](lamb::ConfigVarBase::ptr var){
        LAMB_LOG_INFO(g_logger) << "name=" << var->getName()
            << " description=" << var->getDescription()
            << " typename=" << var->getTypeName()
            << " value=" << var->toString();
    });



    return 0;
}