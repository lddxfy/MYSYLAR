#include "../include/http/ws_server.h"
#include "../include/log.h"
#include "../include/config.h"
#include "../include/env.h"


static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

void run(){
    lamb::http::WSServer::ptr server(new lamb::http::WSServer);
    lamb::Address::ptr addr = lamb::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        LAMB_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    auto fun = [](lamb::http::HttpRequest::ptr header
                  ,lamb::http::WSFrameMessage::ptr msg
                  ,lamb::http::WSSession::ptr session){
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/lamb",fun);
    while(!server->bind(addr)){
        LAMB_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    server->start();
}


int main(int argc, char** argv){
    lamb::EnvMgr::GetInstance()->init(argc,argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());

    lamb::IOManager iom(2);
    iom.schedule(run);
}
