#include "../include/daemon.h"
#include "../include/log.h"
#include "../include/iomanager.h"




static lamb::Logger::ptr g_logger = LAMB_LOG_ROOT();

lamb::Timer::ptr timer;
int server_main(int argc, char **argv) {
    LAMB_LOG_INFO(g_logger) << lamb::ProcessInfoMgr::GetInstance()->toString();
    lamb::IOManager iom(1);
    timer = iom.addTimer(
        1000, []() {
            LAMB_LOG_INFO(g_logger) << "onTimer";
            static int count = 0;
            if (++count > 10) {
                exit(1);
            }
        },
        true);
    return 0;
}

int main(int argc, char **argv) {
    return lamb::start_daemon(argc, argv, server_main, true);
}