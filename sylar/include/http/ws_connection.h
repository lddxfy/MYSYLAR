#include "http_connection.h"
#include "ws_session.h"

namespace sylar{
namespace http{

class WSConnection : public HttpConnection {
public:
    typedef std::shared_ptr<WSConnection> ptr;
    WSConnection(Socket::ptr sock,bool owner = true);
    //根据传入的url封装成uri在调用下面的函数
    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(const std::string& url
                                    ,uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers = {});
    //根据uri的host创建服务器地址，创建sock连接这个服务器，封装HttpRequest(GET)，通过这个sock创建的连接发送这个请求，通过这个sock创建的连接接收响应
    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(Uri::ptr uri
                                    ,uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers = {});

    WSFrameMessage::ptr recvMessage();
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);
    int32_t ping();
    int32_t pong();
};

}
}