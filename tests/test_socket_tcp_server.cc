/**
 * @file test_socket.cc
 * @brief 测试Socket类，tcp服务器
 * @version 0.1
 * @date 2021-09-18
 */
#include "../include/log.h"
#include "../include/socket.h"
#include "../include/address.h"
#include "../include/config.h"
#include "../include/iomanager.h"
#include "../include/env.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

void test_tcp_server() {
    int ret;

    auto addr = lamb::Address::LookupAnyIPAddress("127.0.0.1:12345");
    LAMB_ASSERT(addr);

    auto socket = lamb::Socket::CreateTCPSocket();
    LAMB_ASSERT(socket);

    ret = socket->bind(addr);
    LAMB_ASSERT(ret);
    
    LAMB_LOG_INFO(g_logger) << "bind success";

    ret = socket->listen();
    LAMB_ASSERT(ret);

    LAMB_LOG_INFO(g_logger) << socket->toString() ;
    LAMB_LOG_INFO(g_logger) << "listening...";

    while(1) {
        auto client = socket->accept();
        LAMB_ASSERT(client);
        LAMB_LOG_INFO(g_logger) << "new client: " << client->toString();
        client->send("hello world", strlen("hello world"));
        client->close();
    }
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    lamb::IOManager iom(2);
    iom.schedule(&test_tcp_server);

    return 0;
}