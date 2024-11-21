#include "include/config.h"
#include "include/mutex.h"
#include "include/env.h"
#include "include/util.h"
#include <map>
namespace sylar{

static sylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string& name){
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string& prefix,const YAML::Node& node,std::list<std::pair<std::string,const YAML::Node>>& output){
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")!=std::string::npos){
        MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix,node));
    if(node.IsMap()){
        for(auto it = node.begin();it!=node.end();it++){
            ListAllMember(prefix.empty()?it->first.Scalar() : prefix+"."+it->first.Scalar(),it->second,output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node& root){
    std::list<std::pair<std::string,const YAML::Node>> all_nodes;
    // 将root中的结点进行解析，存放到all_nodes中
    ListAllMember("",root,all_nodes);


    for(auto &i : all_nodes){
        // 遍历，获取key，查找是否包含key，如果包含，将之前修改为从文件中加载的值
        std::string key = i.first;
        if(key.empty()){
            continue;
        }
        //std::cout<<key<<std::endl;
        //std::cout<<i.second<<std::endl;

        std::transform(key.begin(),key.end(),key.begin(),::tolower);
        //查询是否包含key
        ConfigVarBase::ptr var = LookupBase(key);
        
        if(var){
            if(i.second.IsScalar()){
                // 将YAML::内结点值转为Scalar类型
                // 然后从字符串中加载（已通过实现偏特化实现了类型的转换），设置m_val，进行更新
                var->fromString(i.second.Scalar());
            }else{
                // 其他类型 Sequence,偏特化中fromString有对应的处理方法
                /*
                标量（Scalar）：单个的、不可分的值，如字符串、数字或布尔值。
                序列（Sequence）：一系列值，类似于数组或列表，通常用短横线 - 表示列表项。
                映射（Mapping）：键值对的集合，类似于字典，使用冒号 : 分隔键和值。
                */
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }

    }
}
//记录每个文件的修改时间
static std::map<std::string,uint64_t> s_file2modiftime;
static sylar::Mutex s_mutex;

/// 是否强制加载配置文件，非强制加载的情况下，如果记录的文件修改时间未变化，则跳过该文件的加载
void Config::LoadFromConfDir(const std::string &path,bool force){
    std::string absoulte_path = sylar::EnvMgr::GetInstance()->getAbsolutePath(path);
    //std::cout<<absoulte_path<<std::endl;
    std::vector<std::string> files;
    FSUtil::ListAllFile(files,absoulte_path,".yml");

    for(auto &i : files){
        {
            //std::cout<<i<<std::endl;
            struct stat st;
            lstat(i.c_str(),&st);
            sylar::Mutex::Lock lock(s_mutex);
            if(!force && s_file2modiftime[i] == (uint64_t)st.st_mtime){
                continue;
            }
            s_file2modiftime[i] = st.st_mtime;
        }
        try {
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            //MYSYLAR_LOG_INFO(g_logger) << "LoadConfFile file = " << i << " ok";
            //MYSYLAR_LOG_DEBUG(g_logger) << "LoadConfFile file = " << i << " ok";
        }catch(...){
            MYSYLAR_LOG_ERROR(g_logger) << "LoadConfFile file =" << i <<" failed";
        }
    }

}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb){
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap &m = GetDatas();
    for(auto it = m.begin();it !=m.end();it++){
        cb(it->second);
    }
}

}


