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
    this->__poll_ptr = NULL;
}

INetworkTask::~INetworkTask() {
    //因为poller循环中可能会前面的逻辑，引起后面的对象被删除
    //所以析构时做标记，避免引用到无效的对象
    if (this->__poll_ptr != NULL)
        *this->__poll_ptr = NULL;
}

#ifdef __linux__
Epoll::Epoll():epoll_fd(INVALID_FD), max_fd(0), wait_interval(5), events(NULL) {
}

Epoll::~Epoll() {
    if (this->epoll_fd != INVALID_FD)
        close(this->epoll_fd);
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
    int nfds = epoll_wait(this->epoll_fd,
                          this->events, this->max_fd, this->wait_interval);
    for (int i = 0; i < nfds; i++) {
        //TODO: 后面考虑是否有效率更高的做法
        epoll_event *e = &events[i];
        INetworkTask *t = (INetworkTask*)e->data.ptr;
        t->__poll_ptr = &e->data.ptr;
    }

    for (int i = 0; i < nfds; i++) {
        epoll_event *e = &events[i];
        INetworkTask *t = (INetworkTask*)e->data.ptr;
        if (t == NULL)
            continue;
        else
            t->__poll_ptr = NULL;

        if (e->events & EPOLLIN) {
            t->OnRead();
        } else if (e->events & EPOLLOUT) {
            t->OnWrite();
        }
    }
}
#elif defined __APPLE__
Kqueue::Kqueue():kqueue_fd(INVALID_FD), max_fd(0), wait_interval(5e6), events(NULL) {
}

Kqueue::~Kqueue() {
    if (this->kqueue_fd != INVALID_FD)
        close(this->kqueue_fd);
    if (this->events)
        FREE(this->events);
}

bool Kqueue::Init(int max_fd, int interval) {
    if (this->kqueue_fd != INVALID_FD)
        return true;

    this->kqueue_fd = kqueue();
    if (this->kqueue_fd == INVALID_FD)
        return false;

    this->wait_interval = interval * 1e6;
    this->max_fd = max_fd;
    this->events = (struct kevent*)MALLOC(sizeof(struct kevent) * max_fd);

    return this->events != NULL;
}

int Kqueue::Ctl(int fd, int type, int filter, INetworkTask* handler) {
	struct kevent ke[2];
	int nums = 0;

	if(filter != NET_CTL_DEL) {
		int ctl;
		ctl = (type & NET_EVENT_READ) ? EV_ENABLE : EV_DISABLE;
		EV_SET(&ke[nums++], fd, EVFILT_READ, EV_ADD | ctl, 0, 0, handler);

		ctl = (type & NET_EVENT_WRITE) ? EV_ENABLE : EV_DISABLE;
		EV_SET(&ke[nums++], fd, EVFILT_WRITE, EV_ADD | ctl, 0, 0, handler);
	} else {
		EV_SET(&ke[nums++], fd, EVFILT_READ, EV_DELETE, 0, 0, handler);
		EV_SET(&ke[nums++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, handler);
	}

	return kevent(kqueue_fd, ke, nums, NULL, 0, NULL) != -1;
}

void Kqueue::Step() {
	struct timespec timeout = {0, wait_interval};
    int nfds = kevent(kqueue_fd, NULL, 0, events, max_fd, &timeout);

    for (int i = 0; i < nfds; i++) {
        struct kevent *e = &events[i];
        INetworkTask *t = (INetworkTask*)e->udata;

        t->__poll_ptr = &e->udata;
    }

    for (int i = 0; i < nfds; i++) {
    	struct kevent *e = &events[i];
        INetworkTask *t = (INetworkTask*)e->udata;

        if (t == NULL)
            continue;
        else
            t->__poll_ptr = NULL;

        if (e->filter == EVFILT_READ) {
            t->OnRead();
        } else if (e->filter == EVFILT_WRITE) {
            t->OnWrite();
        }
    }
}
#elif defined _WIN32
//Way 1 is define FD_SETSIZE before WinSock2.h
//Way 2 is use soft array, dynamic malloc and cast type to fd_set
//Way 3 is using!
#define MAX_FD_SIZE 1024
struct FakeFDSet {
    u_int fd_count;
    SOCKET fd_array[MAX_FD_SIZE];
    FakeFDSet() {
        this->fd_count = 0;
    }

    FakeFDSet(FakeFDSet &src) {
        doCopy(src);
    }

    FakeFDSet(const FakeFDSet &src) {
        doCopy(src);
    }

    FakeFDSet& operator=(FakeFDSet &src) {
        return *doCopy(src);
    }

    FakeFDSet& operator=(const FakeFDSet &src) {
        return *doCopy(src);
    }

    FakeFDSet* doCopy(const FakeFDSet &src) {
        if(&src != this) {
            this->fd_count = src.fd_count;
            for(int i=0; i<src.fd_count; i++) {
                this->fd_array[i] = src.fd_array[i];
            }
        }
        return this;
    }
};

class SelectEvent {
public:
    SelectEvent() {
        memset(&this->set, 0, sizeof(FakeFDSet));
        memset(&this->handle, 0, sizeof(this->handle) / sizeof(handle[0]));
    }

    ~SelectEvent() {
    }

    void TryRepair() {
        int i = 0;
        u_int &cnt = this->set.fd_count;
        while (i < cnt) {
            int fd = this->set.fd_array[i];
            if (!is_valid_fd(fd)) {
                this->set.fd_array[i] = this->set.fd_array[cnt - 1];
                cnt -= 1;
                log_debug("checek fd:%d is invalid, remove it", fd);
            }
            else {
                i++;
            }
        }
    }

    FakeFDSet* Get() {
        return &this->set;
    }

    INetworkTask* GetHandle(int i) {
        if(this->handle) {
            return this->handle[i];
        }
        return NULL;
    }

    bool Modefiy(int fd, bool watch, INetworkTask *h) {
        if(this->set.fd_count >= MAX_FD_SIZE && this->set.fd_count < 0) {
            return false;
        }
        int i;
        bool find = false;
        for(i=0; i<this->set.fd_count; i++) {
            if(this->set.fd_array[i] == fd) {
                find = true;
                break;
            }
        }

        if(watch) {
            this->set.fd_array[i] = fd; //add
            this->handle[i] = h;
            if(!find) {
                this->set.fd_count++;
            } // else modify
        } else {
            if(find) { //remove
                int end_pos = this->set.fd_count - 1;
                this->set.fd_array[i] = this->set.fd_array[end_pos];
                this->handle[i] = this->handle[end_pos];
                this->set.fd_array[end_pos] = 0;
                this->handle[end_pos] = NULL;
                this->set.fd_count--;
            } //else not exist!
        }

        return true;
    }

private:
    FakeFDSet set;
    INetworkTask *handle[MAX_FD_SIZE];
};

Select::Select() : max_fd(0), read(NULL), write(NULL) {
}

bool Select::Init(int max_fd, int interval) {
    this->wait_interval.tv_sec = interval / 1000;
    this->wait_interval.tv_usec = interval % 1000 * 1000;
    this->read = NEW(SelectEvent);
    this->write = NEW(SelectEvent);
    return this->read && this->write;
}
int Select::Ctl(int fd, int type, int filter, INetworkTask* task) {
    bool watch_read = type & NET_EVENT_READ;
    bool watch_write = type & NET_EVENT_WRITE;
    if (filter == NET_CTL_DEL) {
        watch_read = false;
        watch_write = false;
    }
    bool read = this->read->Modefiy(fd, watch_read, task);
    bool write = this->write->Modefiy(fd, watch_write, task);

    return read && write;
}

void Select::Step() {
    FakeFDSet ori_read; FD_ZERO(&ori_read);
    FakeFDSet ori_write; FD_ZERO(&ori_write);
    FakeFDSet *pr = this->read->Get();
    FakeFDSet *pw = this->write->Get();

    if(pr) {
        ori_read = *pr;
    }
    if(pw) {
        ori_write = *pw;
    }
    FakeFDSet readfds = ori_read;
    FakeFDSet writefds = ori_write;
    //TODO 无效指针问题
    int result = select(0, (fd_set*)&readfds, (fd_set*)&writefds, NULL, &this->wait_interval);

    if(result == SOCKET_ERROR) {
        if (NET_ERRNO == NET_EBADF) {
            log_warning("net exception, error:%d, try repair it\n", WSAGetLastError());
            this->read->TryRepair();
            this->write->TryRepair();
        }
        else {
            log_error("net stop, error:%d\n", WSAGetLastError());
        }
    } else if(result > 0) {
        //read
        for(int i=0; i<ori_read.fd_count; i++) {
            int fd = ori_read.fd_array[i];
            if(FD_ISSET(fd, (fd_set*)&readfds)) {
                INetworkTask *task = this->read->GetHandle(i);
                if(task) {
                    task->OnRead();
                }
            }
        }
        //write
        for(int i=0; i<ori_write.fd_count; i++) {
            int fd = ori_write.fd_array[i];
            if(FD_ISSET(fd, (fd_set*)&writefds)) {
                INetworkTask *task = this->write->GetHandle(i);
                if(task) {
                    task->OnWrite();
                }
            }
        }
    }
}

Select::~Select() {
    SAFE_DELETE(Select, this->read);
    SAFE_DELETE(Select, this->write);
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