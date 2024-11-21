#include "../include/iomanager.h"
#include "../include/tcp_server.h"
#include "../include/log.h"
#include "../include/config.h"
#include "../include/env.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

/**
 * @brief 自定义TcpServer类，重载handleClient方法
 */
class MyTcpServer : public lamb::TcpServer {
protected:
    virtual void handleClient(lamb::Socket::ptr client) override;
};

void MyTcpServer::handleClient(lamb::Socket::ptr client) {
    LAMB_LOG_INFO(g_logger) << "new client: " << client->toString();
    static std::string buf;
    buf.resize(4096);
    client->recv(&buf[0], buf.length()); // 这里有读超时，由tcp_server.read_timeout配置项进行配置，默认120秒
    LAMB_LOG_INFO(g_logger) << "recv: " << buf;
    client->close();
}

void run() {
    lamb::TcpServer::ptr server(new MyTcpServer); // 内部依赖shared_from_this()，所以必须以智能指针形式创建对象
    auto addr = lamb::Address::LookupAny("0.0.0.0:12345");
    LAMB_ASSERT(addr);
    std::vector<lamb::Address::ptr> addrs;
    addrs.push_back(addr);

    std::vector<lamb::Address::ptr> fails;
    while(!server->bind(addrs, fails)) {
        sleep(2);
    }
    
    LAMB_LOG_INFO(g_logger) << "bind success, " << server->toString();

    server->start();
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    lamb::IOManager iom(2);
    iom.schedule(&run);

    return 0;
}