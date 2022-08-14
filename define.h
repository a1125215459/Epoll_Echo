#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

// #include "platform/interface_wrap.h"

#include <errno.h>
#include <time.h>
#include <stdint.h>
// #include "mem_wrap/malloc_wrap.h"


#define NET_STATE_WIFI   1
#define NET_STATE_2G     2
#define NET_STATE_3G     3
#define NET_STATE_4G     4
#define NET_STATE_5G     5
#define NET_DISCONNECT   -1


#define P_ALL     0
#define P_UDP     1
#define P_TCP     2
#define P_QPP     3
#define P_QPP_UDP 4
#define P_NONE    5

#define MAX_UDP_RECV_SIZE  10240
#define MAX_UDP_SEND_SIZE  10240

#if defined(__aarch64__) || defined(__x86_64__) || defined(__MIPSEL__)
#define __system_64__
#undef USE_32_HOOK
#else
#undef __system_64__
#define USE_32_HOOK
#endif

#if defined(__MIPS__)
adfad
#endif
#if defined(__system_64__)
typedef int recv_flag_t;
#else
typedef unsigned int recv_flag_t;
#endif

#define SAFE_DELETE(t, o) if (o != NULL) {DEL(t, o); o = NULL;}
#define NEW_STRING(dest, str)      \
    do {                           \
        int __len = strlen(str);   \
        dest = (char*)MALLOC(__len + 1);\
        strcpy(dest, str);         \
    } while (false)                \

#define BTS(b) b ? "true" : "false"


//毫秒缩写MS
#define MS(t) ((uint64_t)(t)->tv_sec * 1000 + (t)->tv_usec / 1000)

#define CURRENT_MS(r) \
    do {\
        struct timeval __cur; \
        gettimeofday(&__cur, NULL);   \
        r = MS(&__cur); \
    } while (0)

#define DEFINE_SEC(sec) \
    uint32_t sec = time(NULL)          \


//辅助单例的定义和实现
#define SINGLETON_DEFINE(__class_name) public: \
    static __class_name* GetInstance();        \
private: \
    static __class_name* __instance;                \
    __class_name();                                 \
    __class_name(const __class_name&);              \
    __class_name(__class_name&);                    \
    __class_name& operator=(const __class_name&);   \
    __class_name& operator=(__class_name&);


#define SINGLETON_IMPLETE(__class_name) __class_name *__class_name::__instance = NULL; \
    __class_name* __class_name::GetInstance() {                         \
        if (__class_name::__instance == NULL)                           \
            __class_name::__instance = new(__class_name);              \
        return __class_name::__instance;                                \
    }


#endif//__DEFINE_H__
