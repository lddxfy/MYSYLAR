#ifndef __SYLAR_HTTP_HTTP_SERVER_H__
#define __SYLAR_HTTP_HTTP_SERVER_H__

#include "../include/tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace lamb{
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    /**
     * @brief 构造函数
     * 
     * @param keepalive 是否长连接
     * @param worker 工作调度器
     * @param accept_worker 接受连接调度器
     */
    HttpServer(bool keepalive = false,
                lamb::IOManager* worker = lamb::IOManager::GetThis(),
                lamb::IOManager* io_worker = lamb::IOManager::GetThis(),
                lamb::IOManager* accept_worker = lamb::IOManager::GetThis());
    /**
     * @brief 获取ServletDispatch
     * 
     * @return ServletDispatch::ptr 
     */
    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}

    /**
     * @brief 设置ServletDispatch
     */
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    //是否支持长连接
    bool m_isKeepalive;
    // Servlet分发器
    ServletDispatch::ptr m_dispatch;
};

}

}

#endif