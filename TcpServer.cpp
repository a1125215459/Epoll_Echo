#include "socket.h"
#include "network.h"
#include "tcp_test.h"
#include "log.h"
#include <iostream>

using namespace QPPUtils;

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: %s IP Port \n", argv[0]);
        return  -1;
    }

    NetworkPoller *nr = NetworkPoller::GetInstance();
    if (!nr->Init(10240, 2)) {
        log_error("create epoll fd error");
        return -1;
    }
    log_info("start*******************************");
    log_info("create NetworkPoller succ");
    
    TCPServer *ptr = new TCPServer(IP(argv[1], atoi(argv[2])));
    log_info("create new TCPSrv succ");
    while(1){
        nr->Loop();
    }
    delete ptr;
    ptr = nullptr;
    return 0;
}