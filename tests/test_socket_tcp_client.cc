#include "../include/log.h"
#include "../include/socket.h"
#include "../include/address.h"
#include "../include/config.h"
#include "../include/iomanager.h"
#include "../include/env.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();



void test_tcp_client() {
    int ret;

    auto socket = lamb::Socket::CreateTCPSocket();
    LAMB_ASSERT(socket);

    auto addr = lamb::Address::LookupAnyIPAddress("0.0.0.0:12345");
    LAMB_ASSERT(addr);
    //LAMB_LOG_INFO(g_logger) << addr->toString();

    //LAMB_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();

    ret = socket->connect(addr);
    LAMB_ASSERT(ret);

    LAMB_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();

    std::string buffer;
    buffer.resize(1024);
    socket->recv(&buffer[0], buffer.size());
    LAMB_LOG_INFO(g_logger) << "recv: " << buffer;
    socket->close();

    return;
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    lamb::IOManager iom;
    iom.schedule(&test_tcp_client);

    return 0;
}