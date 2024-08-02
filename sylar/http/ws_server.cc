#include "ws_server.h"

namespace sylar{
namespace http{

static sylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("http");

WSServer::WSServer(sylar::IOManager* worker,
                sylar::IOManager* io_worker,
                sylar::IOManager* accept_worker)
                :TcpServer(worker,io_worker,accept_worker)
{
    m_dispatch.reset(new WSServletDispatch);
    m_type = "websocket_server";
}

//首先通过传入的套接字建立一个WSSession连接，然后处理websocket握手过程，得到一个req
//通过得到请求的路径获取servlet，执行这个servlet的连接回调函数，然后在循环中不断的接收消息，并处理
void WSServer::handleClient(Socket::ptr client){
    MYSYLAR_LOG_DEBUG(g_logger) << "handleClient:" << *client;

    WSSession::ptr session(new WSSession(client));
    do{
        HttpRequest::ptr header = session->handleShake();
        if(!header){
            MYSYLAR_LOG_DEBUG(g_logger) << "handleShake error";
            break;
        }

        WSServlet::ptr servlet = m_dispatch->getWSServlet(header->getPath());
        if(!servlet) {
            MYSYLAR_LOG_DEBUG(g_logger) << "no match WSServlet";
            break;
        }
        int rt = servlet->onConnect(header, session);
        if(rt) {
            MYSYLAR_LOG_DEBUG(g_logger) << "onConnect return " << rt;
            break;
        }
        while(true) {
            auto msg = session->recvMessage();
            if(!msg) {
                break;
            }
            rt = servlet->handle(header, msg, session);
            if(rt) {
                MYSYLAR_LOG_DEBUG(g_logger) << "handle return " << rt;
                break;
            }
        }
        servlet->onClose(header,session);
    }while(0);
    session->close();
}


}
}