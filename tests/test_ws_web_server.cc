/**
 * @file test_ws_web_server.cc
 * @brief 网页WebSocket Echo服务器
 *        这个示例用于展示如何结合http和websocket来一起构建网页应用
 *        有一点要注意，实际上http和WebSocket可以共用同一个端口，但lamb未实现这种方式
 * @version 0.1
 * @date 2022-07-26
 */
#include "../include/http/ws_server.h"
#include "../include/log.h"
#include "../include/config.h"
#include "../include/env.h"
#include "../include/http/http_server.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

#define XX(...) #__VA_ARGS__

int32_t handleChat(lamb::http::HttpRequest::ptr req, lamb::http::HttpResponse::ptr rsp, lamb::http::HttpSession::ptr session)
{
    rsp->setHeader("Content-Type", "text/html; charset=UTF-8");
    rsp->setBody(XX(
<html>
    <head>
        <title>WebSocket Echo</title>
    </head>
    <script type="text/javascript">
        var websocket = null;
        // 页面显示消息
        function write_msg(msg) {
            document.getElementById("message").innerHTML += msg + "<br/>";
        }
        // 点击连接WebSocket服务器
        function connect() {
            websocket = new WebSocket("ws://localhost:8021/chat");

            websocket.onerror = function() {
                write_msg("onerror");
            };

            websocket.onopen = function() {
                write_msg("WebSocket服务器连接成功");
            };

            websocket.onmessage= function(msg) {
                //console.log(msg);
                write_msg(msg.data);
            };

            websocket.onclose= function() {
                write_msg("服务器断开");
            };
        };
        // 点击发送消息
        function sendmsg() {
            var msg = document.getElementById('msg').value;
            websocket.send(msg);
        }
    </script>
    <body>
        <button onclick="connect()">连接服务器</button><br/>
        <input id="msg" type="text"/><button onclick="sendmsg()">发送</button><br/>
        <div id="message">
        </div>
    </body>
</html>
));

    return 0;
}

void run_http_server()
{
    lamb::http::HttpServer::ptr server(new lamb::http::HttpServer(true));
    lamb::Address::ptr addr = lamb::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr))
    {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/lamb/chat", handleChat);
    server->start();
}

void run_ws_server()
{
    lamb::http::WSServer::ptr server(new lamb::http::WSServer);
    lamb::Address::ptr addr = lamb::Address::LookupAnyIPAddress("0.0.0.0:8021");
    if (!addr)
    {
        LAMB_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    auto fun = [](lamb::http::HttpRequest::ptr header, lamb::http::WSFrameMessage::ptr msg, lamb::http::WSSession::ptr session)
    {
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/chat", fun);
    while (!server->bind(addr))
    {
        LAMB_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main(int argc, char **argv)
{
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    lamb::IOManager iom(2);
    iom.schedule(run_http_server);
    iom.schedule(run_ws_server);
    return 0;
}