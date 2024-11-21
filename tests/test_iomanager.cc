/**
 * @file test_iomanager.cc 
 * @brief IO协程调度器测试
 * @details 通过IO协程调度器实现一个简单的TCP客户端，这个客户端会不停地判断是否可读，并把读到的消息打印出来
 *          当服务器关闭连接时客户端也退出
 * @version 0.1
 * @date 2021-06-16
 */
#include "../include/log.h"
#include "../include/scheduler.h"
#include "../include/iomanager.h"
#include "../include/env.h"
#include "../include/util.h"
#include "../include/config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

int sockfd;
void watch_io_read();

// 写事件回调，只执行一次，用于判断非阻塞套接字connect成功
void do_io_write() {
    LAMB_LOG_INFO(g_logger) << "write callback";
    int so_err;
    socklen_t len = size_t(so_err);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_err, &len);
    if(so_err) {
        LAMB_LOG_INFO(g_logger) << "connect fail";
        return;
    } 
    LAMB_LOG_INFO(g_logger) << "connect success";
}

// 读事件回调，每次读取之后如果套接字未关闭，需要重新添加
void do_io_read() {
    LAMB_LOG_INFO(g_logger) << "read callback";
    char buf[1024] = {0};
    int readlen = 0;
    readlen = read(sockfd, buf, sizeof(buf));
    if(readlen > 0) {
        buf[readlen] = '\0';
        LAMB_LOG_INFO(g_logger) << "read " << readlen << " bytes, read: " << buf;
    } else if(readlen == 0) {
        LAMB_LOG_INFO(g_logger) << "peer closed";
        close(sockfd);
        return;
    } else {
        LAMB_LOG_ERROR(g_logger) << "err, errno=" << errno << ", errstr=" << strerror(errno);
        close(sockfd);
        return;
    }
    // read之后重新添加读事件回调，这里不能直接调用addEvent，因为在当前位置fd的读事件上下文还是有效的，直接调用addEvent相当于重复添加相同事件
    lamb::IOManager::GetThis()->schedule(watch_io_read);
}

void watch_io_read() {
    LAMB_LOG_INFO(g_logger) << "watch_io_read";
    lamb::IOManager::GetThis()->addEvent(sockfd, lamb::IOManager::READ, do_io_read);
}

void test_io() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    LAMB_ASSERT(sockfd > 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr.s_addr);

    int rt = connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr));
    if(rt != 0) {
        if(errno == EINPROGRESS) {
            LAMB_LOG_INFO(g_logger) << "EINPROGRESS";
            // 注册写事件回调，只用于判断connect是否成功
            // 非阻塞的TCP套接字connect一般无法立即建立连接，要通过套接字可写来判断connect是否已经成功
            lamb::IOManager::GetThis()->addEvent(sockfd, lamb::IOManager::WRITE, do_io_write);
            // 注册读事件回调，注意事件是一次性的
            lamb::IOManager::GetThis()->addEvent(sockfd, lamb::IOManager::READ, do_io_read);
        } else {
            LAMB_LOG_ERROR(g_logger) << "connect error, errno:" << errno << ", errstr:" << strerror(errno);
        }
    } else {
        LAMB_LOG_ERROR(g_logger) << "else, errno:" << errno << ", errstr:" << strerror(errno);
    }
}

void test_iomanager() {
    lamb::IOManager iom;
    //lamb::IOManager iom(10); // 演示多线程下IO协程在不同线程之间切换
    iom.schedule(test_io);
}

int main(int argc, char *argv[]) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());
    
    test_iomanager();

    return 0;
}