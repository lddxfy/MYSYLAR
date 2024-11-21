/**
 * @file test_hook.cc
 * @brief hook模块测试
 * @version 0.1
 * @date 2021-06-21
 */

#include "../include/log.h"
#include "../include/scheduler.h"
#include "../include/iomanager.h"
#include "../include/env.h"
#include "../include/util.h"
#include "../include/config.h"
#include "../include/hook.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

/**
 * @brief 测试sleep被hook之后的浆果
 */
void test_sleep() {
    LAMB_LOG_INFO(g_logger) << "test_sleep begin";
    lamb::IOManager iom;
    
    /**
     * 这里的两个协程sleep是同时开始的，一共只会睡眠3秒钟，第一个协程开始sleep后，会yield到后台，
     * 第二个协程会得到执行，最终两个协程都会yield到后台，并等待睡眠时间结束，相当于两个sleep是同一起点开始的
     */
    iom.schedule([] {
        sleep(2);
        LAMB_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([] {
        sleep(3);
        LAMB_LOG_INFO(g_logger) << "sleep 3";
    });

    LAMB_LOG_INFO(g_logger) << "test_sleep end";
}

/**
 * 测试socket api hook
 */
void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    LAMB_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    LAMB_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LAMB_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    LAMB_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    LAMB_LOG_INFO(g_logger) << buff;
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    //test_sleep();

    

    // 只有以协程调度的方式运行hook才能生效
    lamb::IOManager iom;
    iom.schedule(test_sock);

    LAMB_LOG_INFO(g_logger) << "main end";
    return 0;
}