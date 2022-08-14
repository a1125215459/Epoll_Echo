#include <assert.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#include "socket.h"
#include "log.h"
#include "utils.h"

#define NET_ERRNO errno

#ifdef _WIN32
#define qpp_connect connect
#define qpp_sendto sendto
#define qpp_recvfrom recvfrom
#else
int (*qpp_connect)(int socket, const struct sockaddr *address, socklen_t address_len) = connect;
ssize_t (*qpp_sendto)(int sockfd, const void *buf, size_t len, int flags,
                    const struct sockaddr *dest_addr, socklen_t addrlen) = sendto;

#ifdef NDK
ssize_t (*qpp_send)(int sockfd, const void *buf, size_t len, recv_flag_t flags) = send;
ssize_t (*qpp_recv)(int sockfd, void *buf, size_t len, recv_flag_t flags) = recv;
ssize_t (*qpp_recvfrom)(int sockfd, void *buf, size_t len, recv_flag_t flags,
                        const struct sockaddr *src_addr, socklen_t *addrlen) = recvfrom;
#else
ssize_t (*qpp_send)(int sockfd, const void *buf, size_t len, int flags) = send;
ssize_t (*qpp_recv)(int sockfd, void *buf, size_t len, int flags) = recv;
ssize_t (*qpp_recvfrom)(int sockfd, void *buf, size_t len, int flags,
                      struct sockaddr *src_addr, socklen_t *addrlen) = recvfrom;
#endif//NDK
#endif//_WIN32

int (*global_socket_init_callback)(int) = NULL;
#if defined NDK || defined __APPLE__
NetOperatorCB global_net_operator_callback = NULL;
#endif

#define SOCKET_INIT(s, result) \
    do { \
        if (global_socket_init_callback == NULL) { \
            result = true; \
            break; \
        } \
        result = global_socket_init_callback(s) == 0; \
    } while(0)

namespace QPPUtils {


int udp_create() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1) {
        log_error("socket return error:%s", strerror(NET_ERRNO));
        return INVALID_FD;
    }

    if(!socket_set_nonblock(fd)) {
        log_error("fd:%d set NONBLOCK error:%s", fd, strerror(NET_ERRNO));
        CLOSE(fd);
        return INVALID_FD;
    }

    bool on;
    SOCKET_INIT(fd, on);
    if (!on) {
        log_warning("SOCKET_INIT error, fd:%d", fd);
        CLOSE(fd);
        return INVALID_FD;
    }

    return fd;
}

int udp_listen(QPPUtils::IP ip) {
    int fd = udp_create();
    if (fd == INVALID_FD)
        return INVALID_FD;

    sockaddr_in serveraddr = ip.GetSockAddr();
    if(bind(fd, (sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
        CLOSE(fd);
        log_error("udp bind error:%s port:%d", strerror(NET_ERRNO), ip.port);
        return INVALID_FD;
    }

    return fd;
}

int tcp_listen(QPPUtils::IP ip) {
    sockaddr_in serveraddr = ip.GetSockAddr();
    int nfd = 0;

    if((nfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        log_error("socket error:%s", strerror(NET_ERRNO));
        return INVALID_FD;
    }

    int opt = 1;
    if(setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int)) == -1) {
        CLOSE(nfd);
        log_error("setsockopt SO_REUSEADDR error:%s", strerror(NET_ERRNO));
        return INVALID_FD;
    }

    if (!socket_set_nonblock(nfd)) {
        CLOSE(nfd);
        log_error("set nonblock error");
        return INVALID_FD;
    }

    if(bind(nfd,(sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        CLOSE(nfd);
        log_error("tcp bind error:%s port:%d", strerror(NET_ERRNO), ip.port);
        return INVALID_FD;
    }

    if(listen(nfd, 128) == -1) {
        CLOSE(nfd);
        log_error("listen error:%s", strerror(NET_ERRNO));
        return INVALID_FD;
    }

    IP2STR(ip, ip_str);
    log_info("tcp bind success fd:%d, ip:%s, port:%d", nfd, ip_str, ip.port);
    return nfd;
}

int tcp_connect(QPPUtils::IP ip) {
    int fd = INVALID_FD;

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_error("socket error:%s", strerror(NET_ERRNO));
        return INVALID_FD;
    }

    if (!socket_set_nonblock(fd)) {
        CLOSE(fd);
        log_error("set nonblock error");
        return INVALID_FD;
    }

    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(int));

    bool on;
    SOCKET_INIT(fd, on);
    if (!on) {
        log_warning("SOCKET_INIT error, fd:%d", fd);
        CLOSE(fd);
        return INVALID_FD;
    }

    sockaddr_in addr = ip.GetSockAddr();   
    if(qpp_connect(fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        if(NET_ERRNO == NET_CONNECT_INPROGRESS)
            return fd;

        log_error("socket connect error:%s", strerror(NET_ERRNO));
        CLOSE(fd);
        return INVALID_FD;
    }

    return fd;
}

int get_local_port(int fd) {
	sockaddr_in local;
	socklen_t len = sizeof(local);
	if (getsockname(fd, (sockaddr*)&local, &len) == -1) {
		log_error("getsockname error %s", strerror(NET_ERRNO));
		return -1;
	}
	int port = ntohs(local.sin_port);

	if(0 == port) {
        //如果是UDP socket，没调用send bind之前没有local port，这里bind一个
		struct sockaddr_in si;
		si.sin_family = AF_INET;
		si.sin_port = 0;
		si.sin_addr.s_addr = htonl(INADDR_ANY);
		if(::bind(fd, (sockaddr*)&si, sizeof(si)) < 0) {
			log_error("bind error %s", strerror(NET_ERRNO));
			port = -1;
		} else {
			if (getsockname(fd, (sockaddr*)&si, &len) == -1) {
				log_error("getsockname error %s", strerror(NET_ERRNO));
				return -1;
			}
			port = ntohs(si.sin_port);
		}
	}

	return port;
}

/*
 * 参数 fd:由android分配释放.同时由android设置VPN保护标志.
 */
int networkCheck(int fd) {
    log_debug("set netstat detect fd:%d", fd);
    if(fd < 0) {
        return -1;
    }
    UDPSocket us = UDPSocket::AttachFD(fd);
    //这里目的iP不重要，所以随便写死一个
    DEF_FAKE_IP(fip, "122_224_73_165");
    IP ip(fip, 222);
    int n = us.Sendto("", 1, ip);

    if(n < 0) {
        return -1;
    }

    return 0;
}

bool is_valid_fd(int fd) {
    socklen_t l = sizeof(int);
    int type = 0;

    int r = getsockopt(fd, SOL_SOCKET, SO_TYPE, (char*)&type, &l);
    if (r == -1 && NET_ERRNO == NET_EBADF) {
        return false;
    }
    return true;
}

ip_t ipv4_addr(const char *ip) {
    in_addr s;
    if(inet_pton(AF_INET, ip, &s) == 1)
        return s.s_addr;
    return EMPTY_IP;
}
#ifndef CUT_TCP
int recv_tcp_data(TCPSocket ts, char *buf, int size, bool *need_close) {
    int count = 0;
    *need_close = false;

    do {
        int rn = ts.Recv(buf + count, size - count);
        if (rn > 0) {
            count += rn;
            //读满缓存区就退出
            if (count >= size)
                return count;
        } else if (rn == -1) {
            //有错误时
            if(NET_ERRNO == NET_EINTR)
                continue;
            *need_close = NET_ERRNO != NET_EAGAIN;
            return count;
        } else if (rn == 0) {
            *need_close = true;
            return count;
        }
    } while (true);
}
#endif
//--------------IP-----------
IP::IP() {
    this->ip = EMPTY_IP;
    this->port = 0;
}

IP::IP(const sockaddr_in *addr) {
    if (addr == NULL) {
        this->ip = EMPTY_IP;
        this->port = 0;
    } else {
        this->ip = addr->sin_addr.s_addr;
        this->port = ntohs(addr->sin_port);
    }
}

IP::IP(const char *ip, int port) {
    this->ip = ipv4_addr(ip);
    this->port = port;
}

IP::IP(ip_t ip, int port) {
    this->ip = ip;
    this->port = port;
}

void IP::Clear() {
    this->ip = EMPTY_IP;
    this->port = 0;
}

bool IP::IsEmpty() {
    return this->ip == EMPTY_IP;
}

bool IP::Equals(IP &ip) {
    return this->ip == ip.ip &&
        this->port == ip.port;
}

void IP::IP2Str(char *buf, int len) {
    in_addr in;
    in.s_addr = this->ip;

    inet_ntop(AF_INET, (void*)&in, buf, len);
}

sockaddr_in IP::GetSockAddr() {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->port);
    addr.sin_addr.s_addr = this->ip;

    return addr;
}

int IP::GetSockAddrLen() {
    return sizeof(sockaddr_in);
}

bool IP::IsPrivateAddress(ip_t ip) {
    //A类地址：10.0.0.0～10.255.255.255
    const uint8_t *c = (const uint8_t *)&ip;
    if (c[0] == 10) 
        return true;

    //因为用到了docker自动测试，暂时禁用这个逻辑
    //B类地址：172.16.0.0～172.31.255.255 
    //if (c[0] == 172) {
    //    if (c[1] >= 16 && c[1] <= 31)
    //        return true;
    //}
    
    //C类地址：192.168.0.0～192.168.255.255
    if (c[0] == 192 && c[1] == 168)
        return true;
    //127.0.0.1
    if (c[0] == 127 && c[1] == 0 &&
        c[2] == 0 && c[3] == 1)
        return true;
    return false;
}

ip_t IP::ParseNetworkSegment(ip_t ip) {
    ip_t ret = ip;
    uint8_t *p = (uint8_t*)&ret;
    p[3] = 1;
    return ret;
}


//-------------Socket---------
int Socket::GetFD() {
    return this->fd;
}

bool Socket::IsInvalid() {
    return this->fd == INVALID_FD;
}

bool Socket::IsValid() {
    return this->fd != INVALID_FD;
}

void Socket::Close() {
    if (this->fd != INVALID_FD) {
        CLOSE(this->fd);
    }
}

int Socket::GetLocalPort() {
    return get_local_port(this->fd);
}

bool Socket::SetDFBit(int fd, int flag) {
    
#ifdef __APPLE__
    return false;
#else
    int optval = IP_PMTUDISC_DO;
    
    if (!flag) {
        optval = IP_PMTUDISC_DONT;
    }
    if(setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &optval, sizeof(optval)) == -1) {
        log_info("set fd:%d DF bit error", fd);
        return false;
    }
    return true;
#endif
    
}

bool Socket::IsBlock() {
    //int n = fcntl(this->fd, F_GETFL, 0);
    //return NO_MASK(n, O_NONBLOCK);
    return false;
}

bool Socket::SetNonBlock() {
    return socket_set_nonblock(this->fd);
}

bool Socket::IsAlive() {
    socklen_t len = sizeof(int);
    int soerror = 0;
    int n = getsockopt(this->fd, SOL_SOCKET, SO_ERROR, (char*)&soerror, &len);
    if (n == -1)
        return true;
    if (soerror == 0)
        return true;

    return false;
}

void Socket::SetInitCallback(int (*socket_init_callback)(int)) {
    global_socket_init_callback = socket_init_callback;
}

void Socket::Init() {
    //保留connect函数地址，避免被其它hook类防火墙修改
    //也避免iOS SDK自已hook到自己的connect
#ifndef _WIN32
    qpp_connect = connect;
    qpp_sendto = sendto;
    qpp_recvfrom = recvfrom;
    qpp_send = send;
    qpp_recv = recv;
#endif
}
#if defined NDK || defined __APPLE__
void Socket::SetNetOperatorCallback(NetOperatorCB cb) {
    global_net_operator_callback = cb;
}
#endif
//-----------UDPSocket-----------
UDPSocket UDPSocket::Create() {
    return UDPSocket(udp_create());
}

UDPSocket UDPSocket::Listen(QPPUtils::IP ip) {
    return UDPSocket(udp_listen(ip));
}

UDPSocket UDPSocket::AttachFD(int fd) {
    if (!socket_set_nonblock(fd)) {
        CLOSE(fd);
        fd = INVALID_FD;
    }

    return UDPSocket(fd);
}

UDPSocket::UDPSocket() {
    this->fd = INVALID_FD;
    this->is_out_send = false;
}

UDPSocket::UDPSocket(int fd) {
    this->fd = fd;
    this->is_out_send = false;
}

extern "C" int ios_sendto(int fd, const void *buf, int len, int flag, sockaddr *des, socklen_t l);
int UDPSocket::Sendto(const void *buf, int size, IP ip) {
    //注意，这里只做简单封装，不要影响到errno
    sockaddr_in addr = ip.GetSockAddr();
    int n = 0;
#if defined __APPLE__
    if(this->is_out_send)
        n = ios_sendto(this->fd, buf, size, 0, (sockaddr*)&addr, ip.GetSockAddrLen());
    else
#endif
    n =  qpp_sendto(this->fd, buf, size, 0, (sockaddr*)&addr, ip.GetSockAddrLen());

#if defined NDK || defined __APPLE__
    if( n < 0 && NET_ERRNO == NET_EPERM && global_net_operator_callback)
        global_net_operator_callback("net_state", NET_EPERM);
#endif
    return n;
}

int UDPSocket::Recvfrom(void *buf, int size, IP *ip) {
    //因为申请的移动FD有可能是ipv6，这里申请大一些的addr存储空间
    char addr[64];
    socklen_t len = sizeof(addr);
    int n = qpp_recvfrom(this->fd, buf, size, 0, (sockaddr*)addr, &len);
    if (len == sizeof(sockaddr_in6)) {
        //TODO: 完成ipv6的转换
        *ip = IP();
    } else
        *ip = IP((sockaddr_in*)addr);
    return n;
}
#ifndef CUT_TCP
//-----------TCPSocket-------------
TCPSocket::TCPSocket() {
    this->fd = INVALID_FD;
}

TCPSocket::TCPSocket(int fd) {
    this->fd = fd;
}

TCPSocket TCPSocket::Connect(QPPUtils::IP ip) {
    return TCPSocket(tcp_connect(ip));
}

TCPSocket TCPSocket::AttachFD(int fd) {
    if (!socket_set_nonblock(fd)) {
        CLOSE(fd);
        fd = INVALID_FD;
    }

    return TCPSocket(fd);
}

TCPSocket TCPSocket::InvalidSocket() {
    return TCPSocket();
}

int TCPSocket::Send(const void *buf, int size) {
    int n = send(this->fd, buf, size, 0);
#if defined NDK || defined __APPLE__
    if( n < 0 && NET_ERRNO == NET_EPERM && global_net_operator_callback)
        global_net_operator_callback("net_state", NET_EPERM);
#endif
    return n;
}

int TCPSocket::Recv(void *buf, int size) {
    return recv(this->fd, buf, size, 0);
}

bool TCPSocket::IsConnectSuccess() {
    socklen_t len = sizeof(int);
    int soerror = 0;

    int n = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&soerror, &len);
    return n == 0 && soerror == 0;
}


//--------TCPListenSocket--------
TCPListenSocket::TCPListenSocket() {
    this->fd = INVALID_FD;
}

TCPListenSocket::TCPListenSocket(int fd) {
    this->fd = fd;
}

TCPListenSocket TCPListenSocket::Listen(QPPUtils::IP ip) {
    return TCPListenSocket(tcp_listen(ip));
}

TCPListenSocket TCPListenSocket::AttachFD(int fd) {
    return TCPListenSocket(fd);
}

TCPSocket TCPListenSocket::Accept(IP *ip) {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int new_fd = accept(this->fd, (sockaddr*)&addr, &addr_len);
    if (new_fd == INVALID_FD)
        return TCPSocket();

    if (!socket_set_nonblock(new_fd)) {
        log_info("set fd:%d noblock error", new_fd);
        CLOSE(new_fd);
        return TCPSocket();
    }

    *ip = IP(&addr);
    int flag = 1;
    setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(int));
    return TCPSocket::AttachFD(new_fd);
}
#endif

}
