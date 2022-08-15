#ifndef __QPP_NETWORK_H__
#define __QPP_NETWORK_H__

#include "define.h"
// #include "utils.h"
#include "socket.h"
// #include "timer.h"

#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>
#endif

namespace QPPUtils {

//所有需要网络读写操作的对象，需要实现这个接口
class INetworkTask {
public:
    INetworkTask();
    virtual ~INetworkTask();

    virtual void OnRead() = 0;
    virtual void OnWrite() = 0;

    // void** __poll_ptr;
};

//Poll接口
class IDemultiplexer {
public:
    virtual ~IDemultiplexer() {}
    virtual bool Init(int max_fd, int interval) = 0;
    virtual int Ctl(int fd, int type, int filter, INetworkTask* task) = 0;
    virtual void Step() = 0;
};

#ifdef __linux__
class Epoll: public IDemultiplexer {
public:
	Epoll();
    bool Init(int max_fd, int interval);
	int Ctl(int fd, int type, int filter, INetworkTask* task);
	void Step();
	virtual ~Epoll();
private:
    int epoll_fd;
    int max_fd;
    int wait_interval;
    epoll_event *events;
};
#elif defined __APPLE__
class Kqueue:public IDemultiplexer {
public:
    Kqueue();
    bool Init(int max_fd, int interval);
    int Ctl(int fd, int type, int filter, INetworkTask* task);
    void Step();
    virtual ~Kqueue();
private:
    int kqueue_fd;
    int max_fd;
    int wait_interval;
    struct kevent *events;
};
#elif defined _WIN32
class SelectEvent;
class Select : public IDemultiplexer {
public:
    Select();
    bool Init(int max_fd, int interval);
    int Ctl(int fd, int type, int filter, INetworkTask* task);
    void Step();
    virtual ~Select();
private:
    int max_fd;
    SelectEvent *read;
    SelectEvent *write;
    timeval wait_interval;
};
#endif

class NetworkPoller {
public:
    bool Init(int max_fd, int interval);

    bool Register(int fd, INetworkTask *t, bool read, bool write);
    bool Unregister(int fd);
    bool SetEvent(int fd, INetworkTask *t, bool read, bool write);
    void Loop();
private:
    virtual ~NetworkPoller();
    IDemultiplexer *impl;

    SINGLETON_DEFINE(NetworkPoller);
};
#ifndef CUT_TCP
//-----------TCP Connector-----------
class ITCPConnector {
public:
    virtual ~ITCPConnector() {};
    virtual void OnTCPConnectSuccess(IP ip, TCPSocket ts, int conn_time) = 0;
    virtual void OnTCPConnectFail(IP ip, bool timeout) = 0;
};

class TCPConnector: public INetworkTask {
public:
    TCPConnector(IP ip, int timeout, ITCPConnector *tc);
    void Connect();
    void Close();

    virtual ~TCPConnector();
    virtual void OnRead();
    virtual void OnWrite();

    void OnFailed(bool timeout);
    int GetLocalPort();
private:
    void OnEvent();
    void FreeResource();

    TCPSocket ts;
    // TimerItem *titem;
    // ITCPConnector *tc;
    IP ip;
    // int timeout;
    // bool has_reg;
    // uint64_t start_ms;
};
#endif
}


#endif//__QPP_NETWORK_H__
