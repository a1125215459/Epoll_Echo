#ifndef __QPP_LOG_H__
#define __QPP_LOG_H__

#ifdef NDK
#include <android/log.h>
#include <jni.h>
#endif

#ifdef BUILD_IOS_SDK
#define LOG_TAG_CODE 19
extern "C" {
    extern void __ios_log_print(int level, int tag, const char *fName, const char *fmt, ...);
}
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include<time.h>
#else
#include <sys/time.h>
#endif

//日志级别
#define QPP_LOG_DEBUG 1
#define QPP_LOG_INFO 2
#define QPP_LOG_WARNING 3
#define QPP_LOG_ERROR 4
#define QPP_LOG_FATAL 5

extern int __g_qpp_log_level;

#define TIME2BUF(b) \
    do { \
        time_t vt = time(NULL); \
        struct tm *vtm = localtime(&vt); \
        strftime(b, sizeof(b), "%T", vtm); \
    } while(0)


#define LOG_TAG "SubaoProxy"

#ifdef NDK
#define log_debug(fmt, ...) \
    do { \
        if(QPP_LOG_DEBUG >= __g_qpp_log_level) {\
            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s]  "fmt, __FUNCTION__, ##__VA_ARGS__); \
 } \
    }while(0)
#define log_info(fmt, ...) \
    do { \
        if(QPP_LOG_INFO >= __g_qpp_log_level) {\
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%s]  "fmt, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_warning(fmt, ...) \
    do { \
        if(QPP_LOG_WARNING >= __g_qpp_log_level) {\
            __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%s]  "fmt, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_error(fmt, ...) \
    do { \
        if(QPP_LOG_ERROR >= __g_qpp_log_level) {\
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s]  "fmt, __FUNCTION__, ##__VA_ARGS__);\
        } \
    }while(0)

#define log_fatal(fmt, ...) \
    do { \
        __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "[%s]  "fmt, __FUNCTION__, ##__VA_ARGS__); \
    }while(0)

#elif BUILD_IOS_SDK

#define log_debug(fmt, ...) \
    do { \
        if(QPP_LOG_DEBUG >= __g_qpp_log_level) {\
            __ios_log_print(QPP_LOG_DEBUG, LOG_TAG_CODE, __FUNCTION__, fmt, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_info(fmt, ...) \
    do { \
        if(QPP_LOG_INFO >= __g_qpp_log_level) {\
            __ios_log_print(QPP_LOG_INFO, LOG_TAG_CODE, __FUNCTION__, fmt, ##__VA_ARGS__); \
        }\
    }while(0)

#define log_warning(fmt, ...) \
    do { \
        if(QPP_LOG_WARNING >= __g_qpp_log_level) {\
            __ios_log_print(QPP_LOG_WARNING, LOG_TAG_CODE, __FUNCTION__, fmt, ##__VA_ARGS__); \
        }\
    }while(0)

#define log_error(fmt, ...) \
    do { \
        if(QPP_LOG_ERROR >= __g_qpp_log_level) {\
            __ios_log_print(QPP_LOG_ERROR, LOG_TAG_CODE, __FUNCTION__, fmt, ##__VA_ARGS__); \
        }\
    }while(0)

#define log_fatal(fmt, ...) \
    do { \
        if(QPP_LOG_FATAL >= __g_qpp_log_level) {\
            __ios_log_print(QPP_LOG_FATAL, LOG_TAG_CODE, __FUNCTION__, fmt, ##__VA_ARGS__); \
        }\
    }while(0)

#else

#define log_debug(fmt, ...) \
    do { \
        if(QPP_LOG_DEBUG >= __g_qpp_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s [%s]  " fmt "\n", vtb, __FUNCTION__, ##__VA_ARGS__); \
            fflush(stdout); \
        } \
    }while(0)

#define log_info(fmt, ...) \
    do { \
        if(QPP_LOG_INFO >= __g_qpp_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s [%s]  " fmt "\n", vtb, __FUNCTION__, ##__VA_ARGS__); \
            fflush(stdout); \
        } \
    }while(0)

#define log_warning(fmt, ...) \
    do { \
        if(QPP_LOG_WARNING >= __g_qpp_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s [%s]  " fmt "\n", vtb, __FUNCTION__, ##__VA_ARGS__); \
            fflush(stdout); \
        } \
    }while(0)

#define log_error(fmt, ...) \
    do { \
        if(QPP_LOG_ERROR >= __g_qpp_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s [%s]  " fmt "\n", vtb, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stdout); \
        } \
    }while(0)

#define log_fatal(fmt, ...) \
    do { \
        if(QPP_LOG_FATAL >= __g_qpp_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s [%s]  " fmt "\n", vtb, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stdout); \
        } \
    }while(0)

#endif



#endif//__QPP_LOG_H__
