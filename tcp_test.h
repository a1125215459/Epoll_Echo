#ifndef __TCP_TEST_H__
#define __TCP_TEST_H__

// #include "utils/utils.h"
// #include "qpp/qpp.h"
#include "log.h"
#include "network.h"
#include "data.h"

#define MAXSIZE 1024

using namespace QPPUtils;
// using namespace QPP;

// struct Head {
//     int len;
//     char cmd;
//     char body[0];    
// }__attribute__((packed));
// // sizeof(Head) = 5;

// class Data{
// public:
//     Data(){}
//     ~Data(){}
//     // TODO offset
//     static int writen(int fd, const char *msg, int size){
//         const char* buf = msg;
//         int count = size;
//         while (count > 0)
//         {
//             int len = send(fd, buf, count, 0);
//             if (len == -1)
//             {
//                 close(fd);
//                 return -1;
//             }
//             else if (len == 0)
//             {
//                 continue;
//             }
//             buf += len;
//             count -= len;
//         }
//         return size;
//     }
//     // TODO offset
//     static int readn(int fd, char* buf, int size){
//         char* ptr = buf;
//         int count = size;
//         while (count > 0)
//         {
//             int len = recv(fd, ptr, count, 0);
//             if (len == -1)
//             {
//                 return -1;
//             }
//             else if (len == 0)
//             {
//                 return size - count;
//             }
//             ptr += len;
//             count -= len;
//         }
//         return size;
//     }

//     static const char* packet(char *buf, int len){
//         // TODO packet
//     }

//     // can return anything
//     static const char* unpacket(const char *msg){
//     // Head* unpacket(const char *msg){
//         // TODO unpacket --> return data;
//         Head *head = new Head[sizeof(Head)];    
//         memcpy(&head->len, msg, sizeof(head->len));
//         memcpy(&head->cmd, msg, sizeof(head->cmd));

//     }
// };



class TCPClient{
public:
    TCPClient(IP ip) {
        this->ip = ip;
        this->s = TCPSocket::Connect(this->ip);
    }

    ~TCPClient() {
        this->s.Close();
    }

    virtual void Run() {
        char buf[MAXSIZE];
        fgets(buf, sizeof(buf), stdin);
        int total_len = strlen(buf);
        log_info("total_len is : %d", total_len);
        // TODO packet
        const char *sendbuf = data.packet(buf, total_len);
        // int ret = send(this->s.GetFD(), sendbuf, sizeof(sendbuf), 0);
        int ret = data.writen(this->s.GetFD(), sendbuf, sizeof(sendbuf));
        if (ret > 0){
            log_info("send data len is : %d", ret);
        }
    }

    virtual void OnRead() {
        // TODO unpacket
        // TODO 
    }
private:
    TCPSocket s;
    IP ip;
    Data data;
};



class TCPServer: public INetworkTask {
public:
    TCPServer(IP ip) {
        this->ip = ip;
        this->s = TCPListenSocket::Listen(ip);
        if (!NetworkPoller::GetInstance()->Register(this->s.GetFD(), this, true, false)) {
            log_info("register error");
        }
        log_info("TCPServer Create NetworkPoller Succ");
    }

    ~TCPServer() {
        if (!NetworkPoller::GetInstance()->Unregister(this->s.GetFD())) {
            log_info("unregister error");
        }
        this->s.Close();
    }

    virtual void OnRead() {
        int newfd = this->s.Accept(&ip);
        if (newfd > 0){
            log_info("To new TCPLink");
            new TCPLink(this->ip, newfd);
        }
    }
    
    virtual void OnWrite() {
    }
private:
    TCPListenSocket s;
    IP ip;
};


class TCPLink: INetworkTask{
public:
    TCPLink(IP ip, int fd){
        this->ip = ip;
        this->newfd = fd;
        if (!NetworkPoller::GetInstance()->Register(this->newfd, this, true, false)) {
            log_info("register error");
        }
    }
    ~TCPLink(){}
    // TODO OnRead
    virtual void OnRead(){
        // TODO recv
        // TODO unpacket(recv)
        // TODO Create Cmd
        // TODO handle 

        // send
        // TODO send(handle)
    }

    virtual void OnWrite(){}

private:
    TCPListenSocket s;
    IP ip;
    int newfd;
};


#endif//__TCP_TEST_H__
