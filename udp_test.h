#ifndef __UDP_TEST_H__
#define __UDP_TEST_H__

// #include "utils/utils.h"
// #include "qpp/qpp.h"
#include "log.h"
#include <arpa/inet.h>

using namespace QPPUtils;
// using namespace QPP;

class TestRunnable {
public:
    virtual void Run() = 0;
    virtual ~TestRunnable() {}
};



class UDPClient: public INetworkTask,
                 public TestRunnable {
public:
    UDPClient(IP ip) {
        this->ip = ip;
        this->s = UDPSocket::Create();
        this->last_ms = 0;
        if (!NetworkPoller::GetInstance()->Register(this->s.GetFD(), this, true, false)) {
            log_info("register error");
        }
    }

    virtual ~UDPClient() {
        if (!NetworkPoller::GetInstance()->Unregister(this->s.GetFD())) {
            log_info("unregister error");
        }
        this->s.Close();
    }

    virtual void Run() {
        uint64_t cur_ms;
        CURRENT_MS(cur_ms);
        if (cur_ms - this->last_ms > 50)
            this->last_ms = cur_ms;
        else
            return;
        
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
    uint64_t last_ms;    
    IP ip;
};

class UDPServer: public INetworkTask {
public:
    UDPServer(IP ip) {
        this->s = UDPSocket::Listen(ip);
        if (!NetworkPoller::GetInstance()->Register(this->s.GetFD(), this, true, false)) {
            log_info("register error");
        }
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
        IP recv_ip;
        while ((n = this->s.Recvfrom(buf, sizeof(buf), &recv_ip)) != -1) {
            ip_t ip = *((uint32_t*)buf);
            int port = ntohs(*((uint32_t*)(buf+4)));
            IP dip(ip, port);
            IP2STR(dip, ip_str);
            log_info("server recv %s %d %s:%d", buf, n, ip_str, port);

            this->s.Sendto(buf, n, recv_ip);
        }
    }
    
    virtual void OnWrite() {
    }
private:
    UDPSocket s;
};


#endif//__UDP_TEST_H__
