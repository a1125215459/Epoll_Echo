#ifndef __QPP_SOCKET_H__
#define __QPP_SOCKET_H__


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include "define.h"

#define IP_HEAD_SIZE     20
#define IP_UDP_HEAD_SIZE 28
#define IP_TCP_HEAD_SIZE 40
#define closesocket close

typedef uint32_t ip_t;
typedef uint16_t port_t;

#define CLOSE(x)                                \
    do { \
        while((closesocket(x) == -1) && NET_ERRNO == NET_EINTR); \
        x = INVALID_FD; \
    } while(0)


#define SET_MASK(n, f) ((n) |= f)
#define DEL_MASK(n, f) ((n) &= ~f)
#define HAS_MASK(n, f) (((n) & f) == f)
#define NO_MASK(n, f) (((n) & f) != f)

#define INVALID_FD -1
#define EMPTY_IP 0
#define IP2STR(ip, str) char str[64]; (ip).IP2Str(str, sizeof(str))

#ifndef _WIN32
#ifndef CUT_TCP
extern int (*qpp_connect)(int socket, const struct sockaddr *address, __socklen_t address_len);
#endif//CUT_TCP
extern ssize_t (*qpp_sendto)(int sockfd, const void *buf, size_t len, int flags,
                             const struct sockaddr *dest_addr, __socklen_t addrlen);

#ifdef NDK
extern ssize_t (*qpp_send)(int sockfd, const void *buf, size_t len, recv_flag_t flags);
extern ssize_t (*qpp_recv)(int sockfd, void *buf, size_t len, recv_flag_t flags);
extern ssize_t (*qpp_recvfrom)(int sockfd, void *buf, size_t len, recv_flag_t flags,
                               const struct sockaddr *src_addr, socklen_t *addrlen);
#else
extern ssize_t (*qpp_send)(int sockfd, const void *buf, size_t len, int flags);
extern ssize_t (*qpp_recv)(int sockfd, void *buf, size_t len, int flags);
extern ssize_t (*qpp_recvfrom)(int sockfd, void *buf, size_t len, int flags,
                               struct sockaddr *src_addr, __socklen_t *addrlen);
#endif//NDK
#endif//_WIN32

typedef void (*NetOperatorCB)(const char*, int);

namespace QPPUtils {

class IP {
public:
    explicit IP();
    explicit IP(const sockaddr *addr);
    explicit IP(const char *ip, int port);
    explicit IP(ip_t ip, int port);

    bool Equals(IP &ip);
    bool IsEmpty();
    void Clear();
    sockaddr GetSockAddr();
    int GetSockAddrLen();

    void IP2Str(char *buf, int len);

    ip_t ip;
    int port;

    static bool IsPrivateAddress(ip_t ip);
    static ip_t ParseNetworkSegment(ip_t ip);
};


//封装Socket基本操作
class Socket {
public:
    int GetFD();
    void Close();
    bool IsInvalid();
    bool IsValid();
    bool IsBlock();
    bool IsAlive();
    int GetLocalPort();

    static bool SetDFBit(int fd, int flag);
    static void SetInitCallback(int (*socket_init_callback)(int));
    static void Init();
#if defined NDK || defined __APPLE__
    static void SetNetOperatorCallback(NetOperatorCB cb);
#endif
protected:
    int fd;
private:
    bool SetNonBlock();
};


class UDPSocket: public Socket {
public:
    static UDPSocket Create();
    static UDPSocket Listen(QPPUtils::IP ip);
    static UDPSocket AttachFD(int fd);
    UDPSocket();

    int Sendto(const void *buf, int size, IP ip);
    int Recvfrom(void *buf, int size, IP *ip);
    void SetSendFlag(bool v) {is_out_send = v;}
private:
    bool is_out_send;
    UDPSocket(int fd);
};

#ifndef CUT_TCP
class TCPSocket: public Socket {
public:
    static TCPSocket Connect(QPPUtils::IP ip);
    static TCPSocket AttachFD(int fd);
    static TCPSocket InvalidSocket();
    TCPSocket();

    int Send(const void *buf, int size);
    int Recv(void *buf, int size);
    bool IsConnectSuccess();
private:
    TCPSocket(int fd);
};


class TCPListenSocket: public Socket {
public:
    static TCPListenSocket Listen(QPPUtils::IP ip);
    static TCPListenSocket AttachFD(int fd);
    TCPListenSocket();

    TCPSocket Accept(IP *ip);
private:
    TCPListenSocket(int fd);
};
int recv_tcp_data(TCPSocket ts, char *buf, int size, bool *need_close);
#endif

ip_t ipv4_addr(const char *ip);
int get_local_port(int fd);

int networkCheck(int fd);
bool is_valid_fd(int fd);
}

#endif//__QPP_SOCKET_H__
