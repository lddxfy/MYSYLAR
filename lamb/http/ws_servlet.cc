#include "http/ws_servlet.h"

namespace lamb{
namespace http{

FunctionWSServlet::FunctionWSServlet(callback cb
                ,on_connect_cb connect_cb
                ,on_close_cb close_cb)
                : WSServlet("FunctionWSServlet")
                ,m_callback(cb)
                ,m_connect_cb(connect_cb)
                ,m_close_cb(close_cb){

                }

int32_t FunctionWSServlet::onConnect(lamb::http::HttpRequest::ptr header
                   , lamb::http::WSSession::ptr session){
    if(m_connect_cb){
        return m_connect_cb(header,session);
    }
    return 0;
}

int32_t FunctionWSServlet::onClose(lamb::http::HttpRequest::ptr header
                   , lamb::http::WSSession::ptr session){
    if(m_close_cb){
        return m_close_cb(header,session);
    }
    return 0;
} 

int32_t FunctionWSServlet::handle(lamb::http::HttpRequest::ptr header
                    ,lamb::http::WSFrameMessage::ptr msg
                   , lamb::http::WSSession::ptr session){
    if(m_callback){
        return m_callback(header,msg,session);
    }
    return 0;
}

WSServletDispatch::WSServletDispatch(){
    m_name = "WSservletDispatch";
}

void WSServletDispatch::addServlet(const std::string& uri,
                    FunctionWSServlet::callback cb,
                    FunctionWSServlet::on_connect_cb connect_cb,
                    FunctionWSServlet::on_close_cb close_cb)
{
    ServletDispatch::addServlet(uri,std::make_shared<FunctionWSServlet>(cb,connect_cb,close_cb));
}

void WSServletDispatch::addGlobServlet(const std::string& uri,
                    FunctionWSServlet::callback cb,
                    FunctionWSServlet::on_connect_cb connect_cb,
                    FunctionWSServlet::on_close_cb close_cb)       
{
    ServletDispatch::addGlobServlet(uri,std::make_shared<FunctionWSServlet>(cb,connect_cb,close_cb));
}
WSServlet::ptr WSServletDispatch::getWSServlet(const std::string& uri){
    auto slt = getMatchedServlet(uri);
    return std::dynamic_pointer_cast<WSServlet>(slt);
}
    
}
}