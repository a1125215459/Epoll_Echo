#include "socket.h"
#include "network.h"
#include "tcp_test.h"

using namespace QPPUtils;

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: %s IP Port \n", argv[0]);
        return -1;
    }
    TCPClient *t = new TCPClient(IP(argv[1], atoi(argv[2])));
    log_info("create TCPClient succ");
    while(1){
        t->Run();
        // sleep(2);
        t->OnRead();
    }
    delete t;
    t = nullptr;
}