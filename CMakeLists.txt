cmake_minimum_required(VERSION 3.0)

project(Mysylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
# 指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")
#设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# 添加包含路径
include_directories(${CMAKE_SOURCE_DIR}/sylar)
include_directories(.)

option(BUILD_TEST "ON for complile test" ON)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

link_directories(${PROJECT_SOURCE_DIR}/lib)

set(LIB_SRC 
    sylar/log.cc 
    sylar/config.cc
    sylar/thread.cc
    sylar/mutex.cc
    sylar/env.cc
    sylar/util.cc
    sylar/fiber.cc
    sylar/scheduler.cc
    sylar/iomanager.cc
    sylar/timer.cc
    sylar/fd_manager.cc
    sylar/hook.cc
    sylar/address.cc
    sylar/socket.cc
    sylar/bytearray.cc
    sylar/stream.cc
    sylar/tcp_server.cc
    sylar/http/http-parser/http_parser.c
    sylar/http/http.cc
    sylar/http/http_parser.cc
    sylar/streams/socket_stream.cc
    sylar/http/http_session.cc
    sylar/http/servlet.cc
    sylar/http/http_server.cc
    sylar/uri.cc
    sylar/http/http_connection.cc
    sylar/daemon.cc
    sylar/util/hash_util.cc
    sylar/http/ws_session.cc
    sylar/http/ws_servlet.cc 
    sylar/http/ws_server.cc 
    sylar/http/ws_connection.cc
    )

find_library(YAMLCPP yaml-cpp)
add_library(sylar SHARED ${LIB_SRC})
find_library(PTHREAD pthread)
find_library(RT rt)
find_library(DL dl)

set(
    LIBS
    sylar
    ${DL}
    ${PTHREAD}
    ${YAMLCPP}
    ${RT}
    ${OPENSSL_LIBRARIES}
)


add_executable(test_log tests/log_test.cc)
add_dependencies(test_log sylar)
target_link_libraries(test_log ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIBS})

add_executable(test_env tests/test_env.cc)
add_dependencies(test_env sylar)
target_link_libraries(test_env ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_fiber2 tests/test_fiber2.cc)
add_dependencies(test_fiber2 sylar)
target_link_libraries(test_fiber2 ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager sylar)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_timer tests/test_timer.cc)
add_dependencies(test_timer sylar)
target_link_libraries(test_timer ${LIBS})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook sylar)
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address sylar)
target_link_libraries(test_address ${LIBS})

add_executable(test_socket_tcp_client tests/test_socket_tcp_client.cc)
add_dependencies(test_socket_tcp_client sylar)
target_link_libraries(test_socket_tcp_client ${LIBS})


add_executable(test_socket_tcp_server tests/test_socket_tcp_server.cc)
add_dependencies(test_socket_tcp_server sylar)
target_link_libraries(test_socket_tcp_server ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray sylar)
target_link_libraries(test_bytearray ${LIBS})


add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server sylar)
target_link_libraries(test_tcp_server ${LIBS})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http sylar)
target_link_libraries(test_http ${LIBS})


add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser sylar)
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server sylar)
target_link_libraries(test_http_server ${LIBS})

add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri sylar)
target_link_libraries(test_uri ${LIBS})

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection sylar)
target_link_libraries(test_http_connection ${LIBS})

add_executable(test_daemon tests/test_daemon.cc)
add_dependencies(test_daemon sylar)
target_link_libraries(test_daemon ${LIBS})

add_executable(test_ws_server tests/test_ws_server.cc)
add_dependencies(test_ws_server sylar)
target_link_libraries(test_ws_server ${LIBS})

add_executable(test_ws_client tests/test_ws_client.cc)
add_dependencies(test_ws_client sylar)
target_link_libraries(test_ws_client ${LIBS})

add_executable(test_ws_web_server tests/test_ws_web_server.cc)
add_dependencies(test_ws_web_server sylar)
target_link_libraries(test_ws_web_server ${LIBS})