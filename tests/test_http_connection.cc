#include <iostream>
#include "../include/log.h"
#include "../include/env.h"
#include "../include/http/http_parser.h"
#include "../include/http/http_server.h"
#include "../include/http/http_connection.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

void test_pool() {
    lamb::http::HttpConnectionPool::ptr pool(new lamb::http::HttpConnectionPool(
        "www.midlane.top", "", 80, 10, 1000 * 30, 5));

    lamb::IOManager::GetThis()->addTimer(
        1000, [pool]() {
            auto r = pool->doGet("/", 300);
            std::cout << r->toString() << std::endl;
        },
        true);
}

void run() {
    lamb::Address::ptr addr = lamb::Address::LookupAnyIPAddress("www.midlane.top:80");
    if (!addr) {
        LAMB_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    lamb::Socket::ptr sock = lamb::Socket::CreateTCP(addr);
    bool rt                 = sock->connect(addr);
    if (!rt) {
        LAMB_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    lamb::http::HttpConnection::ptr conn(new lamb::http::HttpConnection(sock));
    lamb::http::HttpRequest::ptr req(new lamb::http::HttpRequest);
    req->setPath("/");
    req->setHeader("host", "www.midlane.top");
    // 小bug，如果设置了keep-alive，那么要在使用前先调用一次init
    req->setHeader("connection", "keep-alive");
    req->init();
    std::cout << "req:" << std::endl
              << *req << std::endl;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        LAMB_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    std::cout << "rsp:" << std::endl
              << *rsp << std::endl;

    std::cout << "=========================" << std::endl;

    auto r = lamb::http::HttpConnection::DoGet("http://www.midlane.top/wiki/", 300);
    std::cout << "result=" << r->result
              << " error=" << r->error
              << " rsp=" << (r->response ? r->response->toString() : "")
              << std::endl;

    std::cout << "=========================" << std::endl;
    test_pool();
}

int main(int argc, char **argv) {
    lamb::IOManager iom(2);
    iom.schedule(run);
    return 0;
}