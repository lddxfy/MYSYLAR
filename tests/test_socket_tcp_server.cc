/**
 * @file test_socket.cc
 * @brief 测试Socket类，tcp服务器
 * @version 0.1
 * @date 2021-09-18
 */
#include "log.h"
#include "socket.h"
#include "address.h"
#include "config.h"
#include "iomanager.h"
#include "env.h"

static sylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void test_tcp_server() {
    int ret;

    auto addr = sylar::Address::LookupAnyIPAddress("127.0.0.1:12345");
    MYSYLAR_ASSERT(addr);

    auto socket = sylar::Socket::CreateTCPSocket();
    MYSYLAR_ASSERT(socket);

    ret = socket->bind(addr);
    MYSYLAR_ASSERT(ret);
    
    MYSYLAR_LOG_INFO(g_logger) << "bind success";

    ret = socket->listen();
    MYSYLAR_ASSERT(ret);

    MYSYLAR_LOG_INFO(g_logger) << socket->toString() ;
    MYSYLAR_LOG_INFO(g_logger) << "listening...";

    while(1) {
        auto client = socket->accept();
        MYSYLAR_ASSERT(client);
        MYSYLAR_LOG_INFO(g_logger) << "new client: " << client->toString();
        client->send("hello world", strlen("hello world"));
        client->close();
    }
}

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    sylar::IOManager iom(2);
    iom.schedule(&test_tcp_server);

    return 0;
}