#include "data.h"
#include <string>
#include <algorithm>

using namespace std;

class Cmd
{
public:
    Cmd(){};
    ~Cmd();
    virtual char *handle(char *buf) = 0;
    Data data;
private:
    /* data */
};

class Echo: public Cmd{
    virtual char *handle(char *buf){
        // TODO
        log_info("here is Echo");
        int len;
        memcpy(&len, buf, 4);
        char *buffer = const_cast<char*>(data.packet(buf, len));
        return buffer;
    }
};

class Upper: public Cmd{
    virtual char *handle(char *buf){
        // TODO Upper
        log_info("here is Upper");
        Head *head = data.unpacket(buf);
        string bbuf;
        char *handle_buf;
        memcpy(&bbuf, head->body, (head->len - 1));
        // bbuf.toupper();
        transform(bbuf.begin(),bbuf.end(), bbuf.begin(), ::toupper);
        strcpy(handle_buf, bbuf.c_str());
        return handle_buf;
    }
};

class CmdFactory{
public:
    // static Cmd *Create(char cmd);
    static Cmd *Create(char *buf);
};

Cmd *CmdFactory::Create(char *buf){
    // int len;
    // memcpy(&len, buf, 4);
    char cmd;
    memcpy(&cmd, buf + 4, 1);
    Cmd *oper;
    switch (cmd)
    {
    case '1':
        oper = new Echo();
        break;
    case '2':
        oper = new Upper();
        break;
    default:
        oper = new Echo();
        break;
    }
    return oper;
}