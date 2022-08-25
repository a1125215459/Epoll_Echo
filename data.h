#include "socket.h"

typedef struct Head {
    int len;
    char cmd;
    char body[0];    
}__attribute__((packed));
// sizeof(Head);

class Data{
public:
    Data(){}
    ~Data(){}
    // TODO offset
    int writen(int fd, const char *msg, int size){
        const char* buf = msg;
        int count = size;
        int offset = 0;
        while (count > 0)
        {
            int len = send(fd, buf + offset, count, 0);
            if (len == -1)
            {
                CLOSE(fd);
                return -1;
            }
            else if (len == 0)
            {
                continue;
            }
            offset += len;
            count -= len;
        }
        return size;
    }
    // TODO offset
    int readn(int fd, char* buf, int size){
        char* ptr = buf;
        int count = size;
        int offset = 0;
        while (count > 0)
        {
            int len = recv(fd, ptr + offset, count, 0);
            if (len == -1)
            {
                CLOSE(fd);
                return -1;
            }
            else if (len == 0)
            {
                // return size - count;
                continue;
            }
            offset += len;
            count -= len;
        }
        return size;
    }

    const char* packet(char *buf, int len){
        // TODO packet
        // total_len + [ cmd + body[0]]
        int total_len = htons(len);
        Head *head = new Head[sizeof(Head)];
        memcpy(&head->len, &total_len, sizeof(head->len));
        log_info("send head->len is: %d", head->len);
        memcpy(&head->cmd, buf, sizeof(head->cmd));
        log_info("send head->cmd is : %s", head->cmd);
        memcpy(head->body, buf + 1, sizeof(buf));
        log_info("send head->body[buf] is :%s", head->body[0]);
        return (const char*)head;        
    }

    // can return anything
    // const char* unpacket(const char *msg){
    Head* unpacket(const char *msg){
        // TODO unpacket --> return data;
        Head *head = new Head[sizeof(Head)];    
        memcpy(&head->len, msg, sizeof(head->len));
        log_info("recv total_len is %d", ntohs(head->len));
        memcpy(&head->cmd, msg + 4, sizeof(head->cmd));
        log_info("recv head->cmd is : %s", head->cmd);
        memcpy(head->body, msg + 5, ntohs(head->len) - 5);
        log_info("recv head->body[buf] is :%s", head->body[0]);
        return head;
    }
};
