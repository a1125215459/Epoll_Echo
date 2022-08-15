#ifndef __UDP_TEST_H__
#define __UDP_TEST_H__

// #include "utils/utils.h"
// #include "qpp/qpp.h"
#include "log.h"
#include <arpa/inet.h>

using namespace QPPUtils;
// using namespace QPP;

// class TestRunnable {
// public:
//     virtual void Run() = 0;
//     virtual ~TestRunnable() {}
// };



class UDPClient: public INetworkTask {
public:
    UDPClient(IP ip) {
        this->ip = ip;
        this->s = UDPSocket::Create();
        log_info("UDPClient Create");
        if (!NetworkPoller::GetInstance()->Register(this->s.GetFD(), this, true, false)) {
            log_info("register error");
        }
        log_info("NetworkPoller create succ");
    }

    virtual ~UDPClient() {
        if (!NetworkPoller::GetInstance()->Unregister(this->s.GetFD())) {
            log_info("unregister error");
        }
        this->s.Close();
    }

    virtual void Run() {     
        char udpbuf[100];
        *((uint32_t*)udpbuf) = ip.ip;
        *((uint16_t*)(udpbuf+4)) = htons(ip.port);
        *((uint8_t*)(udpbuf+6)) = 0;
        int n = 7;

        log_info("send %d", this->exe_count);
        n += snprintf(udpbuf+n, sizeof(udpbuf)-n, "hello udp %d", this->exe_count++);
        udpbuf[n] = 0;

        this->s.Sendto(udpbuf, n+1, ip);
    }

    virtual void OnRead() {
        char buf[32 * 1024];
        int n = 0;
        IP ip;
        while ((n = this->s.Recvfrom(buf, sizeof(buf), &ip)) != -1) {
            log_info("client recv %s %d", buf, n);
        }
    }
    
    virtual void OnWrite() {
    }
private:
    UDPSocket s;
    int exe_count;
    IP ip;
};

class UDPServer: public INetworkTask {
public:
    UDPServer(IP ip) {
        this->ip = ip;
        this->s = UDPSocket::Listen(ip);
        log_info("UDPServer Listen");
        if (!NetworkPoller::GetInstance()->Register(this->s.GetFD(), this, true, false)) {
            log_info("register error");
        }
        log_info("NetworkPoller Register succ!");
    }

    ~UDPServer() {
        if (!NetworkPoller::GetInstance()->Unregister(this->s.GetFD())) {
            log_info("unregister error");
        }
        this->s.Close();
    }

    virtual void OnRead() {
        char buf[32 * 1024];
        int n = 0;
        // IP recv_ip;
        log_info("Start OnRead");
        while ((n = this->s.Recvfrom(buf, sizeof(buf), &ip)) != -1) {
            ip_t ip = *((uint32_t*)buf);
            int port = ntohs(*((uint32_t*)(buf+4)));
            IP dip(ip, port);
            IP2STR(dip, ip_str);
            log_info("server recv %s %d %s:%d", buf, n, ip_str, port);

            this->s.Sendto(buf, n, this->ip);
        }
    }
    
    virtual void OnWrite() {
    }
private:
    UDPSocket s;
    IP ip;
};


#endif//__UDP_TEST_H__
