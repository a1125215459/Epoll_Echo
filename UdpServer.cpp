#include "socket.h"
#include "network.h"
#include "udp_test.h"
#include "log.h"
#include <iostream>

// #define MAX_LISTEN 5
using namespace QPPUtils;

int main(int argc, char *argv[])
{
    if(argc != 3){
        printf("Usage: %s IP Port \n", argv[0]);
        return -1;
    }
    const char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    IP Ip = IP(ip, port);
    NetworkPoller *nr = NetworkPoller::GetInstance();
    if (!nr->Init(10240, 2)) {
        log_error("create epoll fd error");
        return -1;
    }
    log_info("new UDPServer");
    new UDPServer(Ip);
    log_info("Go to Loop");
    while(1){
        nr->Loop();
    }
    

}