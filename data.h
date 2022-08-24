#include "socket.h"

struct Head {
    int len;
    char cmd;
    char body[0];    
}__attribute__((packed));
// sizeof(Head) = 5;

class Data{
public:
    Data(){}
    ~Data(){}
    // TODO offset
    static int writen(int fd, const char *msg, int size){
        const char* buf = msg;
        int count = size;
        while (count > 0)
        {
            int len = send(fd, buf, count, 0);
            if (len == -1)
            {
                close(fd);
                return -1;
            }
            else if (len == 0)
            {
                continue;
            }
            buf += len;
            count -= len;
        }
        return size;
    }
    // TODO offset
    static int readn(int fd, char* buf, int size){
        char* ptr = buf;
        int count = size;
        while (count > 0)
        {
            int len = recv(fd, ptr, count, 0);
            if (len == -1)
            {
                return -1;
            }
            else if (len == 0)
            {
                return size - count;
            }
            ptr += len;
            count -= len;
        }
        return size;
    }

    static const char* packet(char *buf, int len){
        // TODO packet
    }

    // can return anything
    static const char* unpacket(const char *msg){
    // Head* unpacket(const char *msg){
        // TODO unpacket --> return data;
        Head *head = new Head[sizeof(Head)];    
        memcpy(&head->len, msg, sizeof(head->len));
        memcpy(&head->cmd, msg, sizeof(head->cmd));

    }
};
