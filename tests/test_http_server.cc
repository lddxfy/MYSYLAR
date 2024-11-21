#include "../include/http/http_parser.h"
#include "../include/http/http_server.h"
#include "../include/env.h"
#include "../include/config.h"
#include "../include/log.h"

static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

#define XX(...) #__VA_ARGS__

lamb::IOManager::ptr worker;

void run() {
    g_logger->setLevel(lamb::LogLevel::INFO);
    lamb::http::HttpServer::ptr server(new lamb::http::HttpServer(true, worker.get(), lamb::IOManager::GetThis()));
    //lamb::http::HttpServer::ptr server(new lamb::http::HttpServer(true));
    lamb::Address::ptr addr = lamb::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/lamb/xx", [](lamb::http::HttpRequest::ptr req, lamb::http::HttpResponse::ptr rsp, lamb::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/lamb/*", [](lamb::http::HttpRequest::ptr req, lamb::http::HttpResponse::ptr rsp, lamb::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    sd->addGlobServlet("/lambx/*", [](lamb::http::HttpRequest::ptr req, lamb::http::HttpResponse::ptr rsp, lamb::http::HttpSession::ptr session) {
        rsp->setBody(XX(<html>
                                <head><title> 404 Not Found</ title></ head>
                                <body>
                                <center><h1> 404 Not Found</ h1></ center>
                                <hr><center>
                                    nginx /
                                1.16.0 <
                            / center >
                            </ body>
                            </ html> < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >));
        return 0;
    });

    server->start();
}

int main(int argc, char **argv) {
    lamb::EnvMgr::GetInstance()->init(argc, argv);
    lamb::Config::LoadFromConfDir(lamb::EnvMgr::GetInstance()->getConfigPath());
    
    lamb::IOManager iom(1, true, "main");
    worker.reset(new lamb::IOManager(300, false, "worker"));
    iom.schedule(run);
    return 0;
}