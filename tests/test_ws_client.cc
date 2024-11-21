#include "../include/http/ws_server.h"
#include "../include/http/ws_connection.h"
#include "../include/log.h"
#include "../include/config.h"
#include "../include/env.h"
#include "../include/hash_util.h"

void run(){
    auto rt = lamb::http::WSConnection::Create("http://127.0.0.1:8020/lamb",1000);
    if(!rt.second) {
        std::cout << rt.first->toString() << std::endl;
        return;
    }

    auto conn = rt.second;
    while(true){
        for(int i = 0;i<1;i++){
            conn->sendMessage(lamb::random_string(60), lamb::http::WSFrameHead::TEXT_FRAME, false);
        }
        conn->sendMessage(lamb::random_string(65), lamb::http::WSFrameHead::TEXT_FRAME, true);
        auto msg = conn->recvMessage();
        if(!msg){
            break;
        }
        std::cout << "opcode=" << msg->getOpcode()
                  << " data=" << msg->getData() << std::endl;

        sleep(10);
    }
}

int main(int argc, char** argv){
    lamb::EnvMgr::GetInstance()->init(argc,argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());
    //设置种子
    srand(time(0));
    lamb::IOManager iom(1);
    iom.schedule(run);
}