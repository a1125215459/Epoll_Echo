#include "socket.h"
#include "network.h"
#include "udp_test.h"
#include <iostream>

using namespace QPPUtils;


int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: %s IP Port \n", argv[0]);
        return -1;
    }
    const char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    IP Ip = IP(ip, port);
    UDPClient *t = new UDPClient(Ip);
    // NetworkPoller *nr = NetworkPoller::GetInstance();
    // if (!nr->Init(10240, 5)) {
    //     log_error("create epoll fd error");
    //     return -1;
    // }
    while(1){
        // log_info("Run");
        t->Run();//send
        sleep(1);
        // log_info("OnRead");
        t->OnRead();//recv
        // nr->Loop();
    }

    return 0;
}