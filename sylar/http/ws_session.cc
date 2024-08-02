#include "ws_session.h"
#include "../util/hash_util.h"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("http");

sylar::ConfigVar<uint32_t>::ptr g_websocket_message_max_size = sylar::Config::Lookup("websocket.message.max_size",(uint32_t) 1024 * 1024 * 32, "websocket message max size");

WSSession::WSSession(Socket::ptr sock, bool owner) : HttpSession(sock,owner){}
//先接收一个解析完的req请求并对这个请求进行判断看看是否是一个websocket请求，不是则打印req并返回nullptr
//是websocket请求的话，就封装好rsp响应并发送，然后返回req
HttpRequest::ptr WSSession::handleShake(){
    HttpRequest::ptr req;
    do {
        req = recvRequest();

        MYSYLAR_LOG_DEBUG(g_logger) << req;
        if(!req){
            MYSYLAR_LOG_INFO(g_logger) << "invalid http request";
            break;
        }

        if(strcasecmp(req->getHeader("Connection").c_str(),"keep-alive, Upgrade")){
            MYSYLAR_LOG_INFO(g_logger) << "http header Connection != Upgrade";
            break;
        }

        if(strcasecmp(req->getHeader("Upgrade").c_str(),"websocket")){
            MYSYLAR_LOG_INFO(g_logger) << "http header Upgrade != websocket";
            break;
        }
        
        if(req->getHeaderAs<int>("Sec-webSocket-Version")!=13){
            MYSYLAR_LOG_INFO(g_logger) << "http header Sec-webSocket-Version != 13";
            break;
        }
        //随机生成的 base64 码
        std::string key = req->getHeader("Sec-webSocket-Key");
        if(key.empty()){
            MYSYLAR_LOG_INFO(g_logger) << "http header Sec-WebSocket-Key = null";
            break;
        }

        std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        v = sylar::base64encode(sylar::sha1sum(v));
        req->setWebsocket(true);

        auto rsp = req->createResponse();
        rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
        rsp->setWebsocket(true);
        rsp->setReason("Web Socket Protocol Handshake");
        rsp->setHeader("Upgrade", "websocket");
        rsp->setHeader("Connection", "Upgrade");
        rsp->setHeader("Sec-WebSocket-Accept", v);

        sendResponse(rsp);
        MYSYLAR_LOG_DEBUG(g_logger) << *req;
        MYSYLAR_LOG_DEBUG(g_logger) << *rsp;
        return req;
    }while(false);
    if(req){
        MYSYLAR_LOG_INFO(g_logger) << *req;
    }
    return nullptr;

}

WSFrameMessage::WSFrameMessage(int opcode, const std::string& data):m_opcode(opcode),m_data(data){}

std::string WSFrameHead::toString() const{
    std::stringstream ss;
    ss << "[WSFrameHead fin=" << fin
       << " rsv1=" << rsv1
       << " rsv2=" << rsv2
       << " rsv3=" << rsv3
       << " opcode=" << opcode
       << " mask=" << mask
       << " payload=" << payload
       << "]";
    return ss.str();
}

WSFrameMessage::ptr WSSession::recvMessage(){
    return WSRecvMessage(this,false);
}

int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool fin){
    return WSSendMessage(this,msg,false,fin);

}

int32_t WSSession::sendMessage(const std::string& msg, int32_t opcode, bool fin){
    return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, fin);
}
    
int32_t WSSession::ping(){
    return WSPing(this);
}
//先将websocket头部通过设置的opcode操作码进行解析，
//然后将读到的数据封装成WSFrameMessage::ptr返回
WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client){
    int opcode = 0;
    std::string data;
    int cur_len = 0;
    do {
        WSFrameHead ws_head;
        //读固定长度的头部
        if(stream->readFixSize(&ws_head,sizeof(ws_head)) <= 0){
            break;
        }
        //打印发送的websocket请求的头部字段
        MYSYLAR_LOG_DEBUG(g_logger) << "WSFrameHead " << ws_head.toString();
        //收到客户端的PING控制帧，应该回一个PONG
        if(ws_head.opcode == WSFrameHead::PING) {
            MYSYLAR_LOG_INFO(g_logger) << "PING";
            if(WSPong(stream) <= 0) {
                break;
            }
        } 
        else if(ws_head.opcode == WSFrameHead::PONG) {
        } 
        else if(ws_head.opcode == WSFrameHead::CONTINUE
                || ws_head.opcode == WSFrameHead::TEXT_FRAME
                || ws_head.opcode == WSFrameHead::BIN_FRAME) {
            
            if(!client && !ws_head.mask){
                MYSYLAR_LOG_INFO(g_logger) << "WSFrameHead mask != 1";
                break;
            }

            uint64_t length = 0;
            //payload字段是126，就读7+16bit
            if(ws_head.payload == 126){
                uint16_t len = 0;
                if(stream->readFixSize(&len,sizeof(len)) <= 0){
                    break;
                }
                length = sylar::byteswapOnLittleEndian(len);
            }//payload字段是127,就读7+64bit
            else if(ws_head.payload == 127){
                uint64_t len = 0;
                if(stream->readFixSize(&len,sizeof(len)) <= 0){
                    break;
                }
                length = sylar::byteswapOnLittleEndian(len);
            }//小于126,就直接把长度设置为payload
            else {
                length = ws_head.payload;
            }

            if((cur_len + length) >= g_websocket_message_max_size->getValue()) {
                MYSYLAR_LOG_WARN(g_logger) << "WSFrameMessage length > "
                    << g_websocket_message_max_size->getValue()
                    << " (" << (cur_len + length) << ")";
                break;
            }
            //Mask-key
            char mask[4] = {0};
            if(ws_head.mask) {
                if(stream->readFixSize(mask, sizeof(mask)) <= 0) {
                    break;
                }
            }
            //Payload数据
            data.resize(cur_len + length);
            if(stream->readFixSize(&data[cur_len], length) <= 0) {
                break;
            }
            //设置了MASK字段，就将发送的数据进行掩码
            if(ws_head.mask) {
                for(int i = 0; i < (int)length; ++i) {
                    data[cur_len + i] ^= mask[i % 4];
                }
            }
            cur_len += length;
            
            if(!opcode && ws_head.opcode != WSFrameHead::CONTINUE) {
                opcode = ws_head.opcode;
            }
            //表示这是消息的最后一个数据帧，打印发送的消息
            if(ws_head.fin) {
                MYSYLAR_LOG_DEBUG(g_logger) << data;
                return WSFrameMessage::ptr(new WSFrameMessage(opcode, std::move(data)));
            }
        }else if(ws_head.opcode == WSFrameHead::CLOSE){
            MYSYLAR_LOG_DEBUG(g_logger) << "opcode=8, close"; // 这里忽略剩下的4个mask字节也没关闭
            break;
        }else {
            MYSYLAR_LOG_DEBUG(g_logger) << "invalid opcode=" << ws_head.opcode;
        }
    }while(true);
    stream->close();
    return nullptr;
}
//先设置帧头部字段并将头部发送出去，然后通过payload字段的值看是否需要发送额外的长度信息，
//然后根据client，如果为true则需要生成一个掩码并应用到数据上。使用 rand 生成掩码，并对数据进行 XOR 操作。
//最后发送经过掩码处理过的数据，如果消息成功发送，返回发送的总字节数（帧头加上数据）
int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin){
    do {
        WSFrameHead ws_head;
        memset(&ws_head,0,sizeof(ws_head));
        ws_head.fin = fin;
        ws_head.opcode = msg->getOpcode();
        ws_head.mask = client;
        uint64_t size = msg->getData().size();
        if(size < 126) {
            ws_head.payload = size;
        } else if(size < 65536) {
            ws_head.payload = 126;
        } else {
            ws_head.payload = 127;
        }

        if(stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }

        if(ws_head.payload == 126) {
            uint16_t len = size;
            len = sylar::byteswapOnLittleEndian(len);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        }else if(ws_head.payload == 127) {
            uint16_t len = sylar::byteswapOnLittleEndian(len);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        }
        if(client){
            char mask[4];
            uint32_t rand_value = rand();
            memcpy(mask, &rand_value, sizeof(mask));

            std::string& data = msg->getData();
            for(size_t i = 0; i < data.size(); ++i) {
                data[i] ^= mask[i % 4];
            }
            if(stream->writeFixSize(mask, sizeof(mask)) <= 0) {
                break;
            }
        }
        if(stream->writeFixSize(msg->getData().c_str(), size) <= 0) {
            break;
        }
        return size + sizeof(ws_head);
    }while(0);
    stream->close();
    return -1;
}

int32_t WSSession::pong(){
    return WSPong(this);
}

int32_t WSPing(Stream* stream){
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PING;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0) {
        stream->close();
    }
    return v;
}
//WebSocket 协议规定，在接收到客户端的 PING 控制帧后，
//服务器应该回复一个 PONG 控制帧
//PING 和 PONG 控制帧主要用于心跳检测和保活机制，以确保连接的持续性和稳定性。
int32_t WSPong(Stream* stream){
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PONG;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0) {
        stream->close();
    }
    return v;
}

}
}