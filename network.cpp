#include "network.h"
#include <unistd.h>

#define DEL(T, ptr) delete ptr
#define NEW(T, ...) new T(__VA_ARGS__)
#define FREE(P) free(P)

namespace QPPUtils {

enum {
    NET_EVENT_READ = 1,
    NET_EVENT_WRITE = 2
};

enum {
    NET_CTL_ADD = 1,
    NET_CTL_DEL = 2,
	NET_CTL_MOD = 4
};


INetworkTask::INetworkTask() {
    // this->__poll_ptr = NULL;
}

INetworkTask::~INetworkTask() {
    //因为poller循环中可能会前面的逻辑，引起后面的对象被删除
    //所以析构时做标记，避免引用到无效的对象
    // if (this->__poll_ptr != NULL)
        // *this->__poll_ptr = NULL;
}

#ifdef __linux__
Epoll::Epoll():epoll_fd(INVALID_FD), max_fd(0), wait_interval(5), events(NULL) {
}

Epoll::~Epoll() {
    if (this->epoll_fd != INVALID_FD)
        CLOSE(this->epoll_fd);
    if (this->events)
        FREE(this->events);
}

bool Epoll::Init(int max_fd, int interval) {
    if (this->epoll_fd != INVALID_FD)
        return true;

    this->epoll_fd = epoll_create(max_fd);
    if (this->epoll_fd == INVALID_FD)
        return false;

    this->wait_interval = interval;
    this->max_fd = max_fd;
    this->events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * max_fd);

    return true;
}

int Epoll::Ctl(int fd, int type, int filter, INetworkTask* task) {
	int ctl = 0;
	struct epoll_event ev;

	ev.data.fd = fd;
	ev.data.ptr = task;
	ev.events = 0;
	if(filter == NET_CTL_ADD) {
		ctl = EPOLL_CTL_ADD;
	} else if(filter == NET_CTL_DEL) {
		ctl = EPOLL_CTL_DEL;
	} else {
		ctl = EPOLL_CTL_MOD;
	}

	if(type & NET_EVENT_READ) {
		ev.events |= EPOLLIN;
	}
	if(type & NET_EVENT_WRITE) {
		ev.events |= EPOLLOUT;
	}

	return epoll_ctl(this->epoll_fd, ctl, fd, &ev) != -1;
}

void Epoll::Step() {
    // log_info("here is Step");
    int nfds = epoll_wait(this->epoll_fd,
                          this->events, this->max_fd, this->wait_interval);
    // for (int i = 0; i < nfds; i++) {
    //     //TODO: 后面考虑是否有效率更高的做法
    //     epoll_event *e = &events[i];
    //     INetworkTask *t = (INetworkTask*)e->data.ptr;
        // t->__poll_ptr = &e->data.ptr;
    // }
    // if (nfds > 0){
    //     log_info("nfds is %d", nfds);    
    // }
    for (int i = 0; i < nfds; i++) {
        epoll_event *e = &events[i];
        INetworkTask *t = (INetworkTask*)e->data.ptr;
        // if (t == NULL)
        //     continue;
        // else
            // t->__poll_ptr = NULL;

        if (e->events & EPOLLIN) {
            // log_info("next is t->OnRead");
            t->OnRead();
        } else if (e->events & EPOLLOUT) {
            t->OnWrite();
        }
    }
}
#endif

//-------------NetworkPoller-----------
SINGLETON_IMPLETE(NetworkPoller)

NetworkPoller::NetworkPoller():impl(0) {
}

NetworkPoller::~NetworkPoller() {
    if (this->impl) {
        DEL(IDemultiplexer, this->impl);
    }
}

bool NetworkPoller::Init(int max_fd, int interval) {
#ifdef __linux__
    impl = NEW(Epoll);
#elif __APPLE__
    impl = NEW(Kqueue);
#elif _WIN32
    impl = NEW(Select);
#else
#error "Not implement yet!"
#endif
    log_info("NetworkPoller Init succ");
    return impl ? impl->Init(max_fd, interval) : false;
}

bool NetworkPoller::Register(int fd, INetworkTask *t, bool read, bool write) {
	int type = 0;

	if(read) {
		type |= NET_EVENT_READ;
	}
	if(write) {
		type |= NET_EVENT_WRITE;
	}

	return impl->Ctl(fd, type, NET_CTL_ADD, t);
}

bool NetworkPoller::Unregister(int fd) {
	int type = NET_EVENT_READ | NET_EVENT_WRITE;
	return impl->Ctl(fd, type, NET_CTL_DEL, NULL);
}

bool NetworkPoller::SetEvent(int fd, INetworkTask *t, bool read, bool write) {
	int type = 0;

	if(read) {
		type |= NET_EVENT_READ;
	}
	if(write) {
		type |= NET_EVENT_WRITE;
	}

    return impl->Ctl(fd, type, NET_CTL_MOD, t);
}

void NetworkPoller::Loop() {
    impl->Step();
}
}