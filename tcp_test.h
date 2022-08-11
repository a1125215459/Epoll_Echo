#ifndef __TCP_TEST_H__
#define __TCP_TEST_H__

#include "utils/utils.h"
#include "qpp/qpp.h"

using namespace QPPUtils;
using namespace QPP;

class TCPClient: public ITaskEventCallback,
                 public TestRunnable {
public:
    TCPClient(Env *e, IP ip) {
        this->remote_ip = ip;
        this->e = e;
        this->t = ClientTCPTask::Connect(e, ip, 5);
        this->t->SetEventCallback(this);
        this->send_count = 0;

        this->exe_count = 0;
        this->active = false;
        this->closed = false;

        this->last_ms = 0;
    }

    ~TCPClient() {
        QPP::FreeTask(this->t);
    }

    virtual void Run() {
        if (this->closed)
            return;
        if (!active)
            return;

        uint64_t cur_ms;
        CURRENT_MS(cur_ms);
        if (cur_ms - this->last_ms > 20)
            this->last_ms = cur_ms;
        else
            return;

        if (exe_count == 0) {

        }

        if (this->exe_count >= 3000) {
            log_info("close");
            this->t->Close();
            this->closed = true;
        }
        this->exe_count++;
        return;
    }

    virtual void OnEvent(Task *t) {
        int n = 0;
        char buf[320000];
        if (send_count >= sizeof(buf)) return;
        do {
            n = this->t->Send(buf + send_count, sizeof(buf) - send_count);
            if (n > 0) {
                log_info("OnEvent cur:%d send:%d total:%d", send_count, n, send_count + n);
                send_count +=  n;
            } else
                log_info("OnEvent send error %d", n);
        } while (n > 0);

        if (!active) {
            TCPTask *tt = (TCPTask*)t;
            if (tt->State() == QTS_ESTABLISHED) {
                this->active = true;
            } else
                return;
        }

        n = 0;
        while ((n = this->t->Recv(buf, sizeof(buf))) > 0) {
            log_info("recv: %s %d", buf, n);
        }

    }
private:
    Env *e;
    ClientTCPTask *t;
    uint64_t last_ms;
    int exe_count;
    bool active;
    bool closed;
    IP remote_ip;

    int send_count;
};



class TCPServer: public INetworkTask, ITaskEventCallback {
public:
    TCPServer(Env *e, IP ip) {
        this->e = e;
        this->ul = e->CreateTCPListener(ip);
        if (!NetworkPoller::GetInstance()->Register(ul->GetSocket().GetFD(), this, true, false)) {
            log_info("register error");
        }
        this->recv_total = 0;
    }

    ~TCPServer() {
        if (!NetworkPoller::GetInstance()->Unregister(ul->GetSocket().GetFD())) {
            log_info("unregister error");
        }
        e->FreeTCPListener(this->ul);
    }

    virtual void OnEvent(Task *t) {
        int n = 0;
        char buf[1024];

        while ((n = t->Recv(buf, sizeof(buf))) > 0) {
            this->recv_total += n;
            log_info("recv:%d total:%d", recv_total, n);
            //t->Send(buf, n);
        }

        if (n == QPP_CLOSE) 
            t->Close();
    }

    virtual void OnRead() {
        this->ul->OnRecvEvent();

        TCPTask *ut = NULL;
        while ((ut = this->ul->Accept()) != NULL) {
            ut->SetEventCallback(this);
            log_info("new task %p", ut);
        }
    }
    
    virtual void OnWrite() {
    }
private:
    Env *e;
    TCPListener *ul;
    int recv_total;
};



#endif//__TCP_TEST_H__
