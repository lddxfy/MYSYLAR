#ifndef __SYLAR_HTTP_WS_SESSION_H__
#define __SYLAR_HTTP_WS_SESSION_H__

#include "../include/config.h"
#include "http_session.h"
#include <stdint.h>


namespace lamb{
namespace http{

//告诉编译器将结构体或类的每个成员按照1字节边界对齐，
//这样可以保证结构体占用的内存空间正好等于其成员占用的总和。
#pragma pack(1)
struct WSFrameHead 
{
    enum OPCODE{
        /// 数据分片帧
        CONTINUE = 0,
        /// 文本帧
        TEXT_FRAME = 1,
        /// 二进制帧
        BIN_FRAME = 2,
        /// 断开连接
        CLOSE = 8,
        /// PING
        PING = 0x9,
        /// PONG
        PONG = 0xA
    };
    //这个是用来标志这是个什么类型的数据帧，表示占4位
    uint32_t opcode: 4;
    bool rsv3: 1;
    bool rsv2: 1;
    bool rsv1: 1;
    //在WebSocket通信中，消息可以被分割成多个帧进行发送，
    //而 "fin" 字段的作用就是告诉对方是否已经发送完毕，或者是否还有后续的帧。
    bool fin: 1;
    //存放的是我们真正想要传输的数据的长度。
    uint32_t payload: 7;
    //指示WebSocket帧的负载（payload）是否被掩码（masked）
    bool mask: 1;

    std::string toString() const;
};
#pragma pack()

class WSFrameMessage {
public:
    typedef std::shared_ptr<WSFrameMessage> ptr;
    WSFrameMessage(int opcode = 0, const std::string& data = "");

    int getOpcode() const { return m_opcode;}
    void setOpcode(int v) { m_opcode = v;}

    const std::string& getData() const { return m_data;}
    std::string& getData() { return m_data;}
    void setData(const std::string& v) { m_data = v;}
private:
    int m_opcode;
    std::string m_data;

};

class WSSession : public HttpSession {
public:
    typedef std::shared_ptr<WSSession> ptr;
    WSSession(Socket::ptr sock, bool owner = true);

    /// server client 处理WebSocket握手过程，返回一个HttpRequest对象。接收req请求，发送rsp响应
    HttpRequest::ptr handleShake();
    //接收WebSocket消息，返回一个WSFrameMessage对象。
    WSFrameMessage::ptr recvMessage();
    //发送WebSocket消息，可以是文本或二进制数据，并且可以指定是否是消息的最后一帧。
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);
    //发送WebSocket ping和pong控制帧，用于检测连接的活性。
    int32_t ping();
    int32_t pong();
private:
    //分别用于处理服务器端和客户端的握手过程
    bool handleServerShake();
    bool handleClientShake();

};

//用于设置WebSocket消息的最大大小。
extern lamb::ConfigVar<uint32_t>::ptr g_websocket_message_max_size;
//是处理WebSocket消息接收和发送的外部函数，它们接受一个Stream对象和相关参数。
WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client);
int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin);
//发送WebSocket ping和pong控制帧的外部函数。
int32_t WSPing(Stream* stream);
int32_t WSPong(Stream* stream);




}
}

#endif