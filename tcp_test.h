#ifndef __TCP_TEST_H__
#define __TCP_TEST_H__

// #include "utils/utils.h"
// #include "qpp/qpp.h"
#include "log.h"
#include "network.h"
#include "data.h"
#include "cmd.h"

#define MAXSIZE 1024

using namespace QPPUtils;


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
        // total_len don't have itself
        // total_len = cmd_len + body_len
        int total_len = strlen(buf);
        log_info("total_len is : %d", total_len);
        // TODO packet
        // sendbuf = total_len + cmd + body
        const char *sendbuf = data.packet(buf, total_len);
        // int ret = send(this->s.GetFD(), sendbuf, sizeof(sendbuf), 0);
        int ret = data.writen(this->s.GetFD(), sendbuf, sizeof(sendbuf));
        if (ret > 0){
            log_info("send data len is : %d", ret);
        }
    }

    virtual void OnRead() {
        // TODO recv
        char recvbuf[MAXSIZE];
        int ret = data.readn(this->s.GetFD(), recvbuf, sizeof(recvbuf));
        // TODO if ret
        if (ret > 0){
            log_info("recv data ret is: %d", ret);
        }
        // TODO unpacket
        Head *head = data.unpacket(recvbuf);
        log_info("len is : %d, cmd is %d, data is : %s", head->len, head->cmd, head->body);
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
        log_info("Create TCPServer Succ");
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
        log_info("Create new TCPLink Succ");
    }
    ~TCPLink(){
        if (!NetworkPoller::GetInstance()->Unregister(this->newfd)) {
            log_info("unregister error");
        }
        // this->s.Close();
        CLOSE(this->newfd);
    }
    // TODO OnRead
    virtual void OnRead(){
        // TODO recv
        char recvbuf[MAXSIZE];
        int ret = data.readn(this->newfd, recvbuf, sizeof(recvbuf));
        if (ret > 0){
            log_info("recv data ret is: %d", ret);
        }

        // TODO unpacket(recv)
        // const char *msg = data.unpacket(recvbuf);
        // Head *head = data.unpacket(recvbuf);

        // TODO Create Cmd
        // Cmd *oper = CmdFactory::Create(head->cmd);
        Cmd *oper = CmdFactory::Create(recvbuf);
        // TODO handle
        // char *body = head->body;
        const char *sendbuf = oper->handle(recvbuf);
        // TODO send
        ret = data.writen(this->newfd, sendbuf, sizeof(sendbuf));
        if (ret > 0){
            log_info("send data ret is: %d", ret);
        }
    }

    virtual void OnWrite(){}

private:
    // TCPListenSocket s;
    int newfd;
    IP ip;
    Data data;
};


#endif//__TCP_TEST_H__
