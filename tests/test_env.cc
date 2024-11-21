#include "../include/log.h"
#include "../include/env.h"
#include "../include/util.h"

lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

lamb::Env *g_env = lamb::EnvMgr::GetInstance();

int main(int argc,char *argv[]){
    g_env->addHelp("h","print this help message");

    bool is_print_help = false;
    if(!g_env->init(argc,argv)){
        is_print_help = true;
    }

    if(g_env->has("h")){
        is_print_help = true;
    }

    if(is_print_help) {
        g_env->printHelp();
    }

    LAMB_LOG_INFO(g_logger)<< "exe: " << g_env->getExe();
    LAMB_LOG_INFO(g_logger) <<"cwd: " << g_env->getCwd();
    LAMB_LOG_INFO(g_logger) << "absoluth path of test: " << g_env->getAbsolutePath("test");
    LAMB_LOG_INFO(g_logger) << "conf path:" << g_env->getConfigPath();

    g_env->add("key1", "value1");
    LAMB_LOG_INFO(g_logger) << "key1: " << g_env->get("key1");

    g_env->setEnv("key2", "value2");
    LAMB_LOG_INFO(g_logger) << "key2: " << g_env->getEnv("key2");

    LAMB_LOG_INFO(g_logger) << g_env->getEnv("PATH");

    return 0;
}