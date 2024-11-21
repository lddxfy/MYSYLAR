#include "../include/log.h"
#include "../include/socket.h"
#include "../include/address.h"
#include "../include/config.h"
#include "../include/iomanager.h"
#include "../include/env.h"

static sylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();



void test_tcp_client() {
    int ret;

    auto socket = sylar::Socket::CreateTCPSocket();
    MYSYLAR_ASSERT(socket);

    auto addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:12345");
    MYSYLAR_ASSERT(addr);
    //MYSYLAR_LOG_INFO(g_logger) << addr->toString();

    //MYSYLAR_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();

    ret = socket->connect(addr);
    MYSYLAR_ASSERT(ret);

    MYSYLAR_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();

    std::string buffer;
    buffer.resize(1024);
    socket->recv(&buffer[0], buffer.size());
    MYSYLAR_LOG_INFO(g_logger) << "recv: " << buffer;
    socket->close();

    return;
}

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    sylar::IOManager iom;
    iom.schedule(&test_tcp_client);

    return 0;
}