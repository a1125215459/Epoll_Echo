#ifndef __QPP_UTILS_H__
#define __QPP_UTILS_H__

#include "define.h"
// #include "list.h"
#include "log.h"
// #include "timer.h"
// #include "khash.h"
// #include "event.h"
// #include "protocol.h"
// #include "mem_wrap/malloc_wrap.h"

namespace QPPUtils {

void set_log_level(int l);
void fakeip2ip(const char *fip, char *ip);
void xor_crypt(const char *data, int data_len, char *codes);

//用宏定义IP，避免iOS发现APP里有hardcode的IP
#define DEF_FAKE_IP(ip, fake_address) char ip[32]; fakeip2ip(fake_address, ip);

class IWriter {
public:
    virtual ~IWriter();
    virtual int Write(const char *data, int size) = 0;
};

class IReader {
    virtual ~IReader();
    virtual int Read(char *data, int size) = 0;
};
    
//------------GlobalTimer-------------
class GlobalTimer: public Timer {
private:
    virtual ~GlobalTimer();

    SINGLETON_DEFINE(GlobalTimer);
};

//----------AutoFree---------------
class IAutoFree {
public:
    IAutoFree();
    virtual ~IAutoFree();
    virtual bool IsActive() = 0;

    list_head __free_node;
};

class AutoFreeManager {
public:
    void Add(IAutoFree *a);
    void Loop();
private:
    ~AutoFreeManager();
    list_head list;

    SINGLETON_DEFINE(AutoFreeManager);
};

class EventManager {
public:
    int ExecuteC2LEvent(Event *e);
    int ExecuteC2LEvent(Event *e, char *buf, int size, int idx);
    Event* PopC2LEvent();
    void ExecuteL2CEvent(Event *e);
    Event* PopL2CEvent();

    void OnFreeEvent(Event *e);

    void Close();
private:
    ~EventManager();

    bool active;
    Event *cur_event;
    ConcurrentList<Event> c2l_list;
    ConcurrentList<Event> l2c_list;
    SINGLETON_DEFINE(EventManager);
};


template<typename T>
class Vector {
public:
    Vector() {
        this->cap = 10;
        this->list = NEWN(T, this->cap);
        this->size = 0;
    }
    ~Vector() {
        DELETEN(T, this->cap, this->list);
    }

    int Size() {
        return size;
    }

    void Clear() {
        this->size = 0;
    }

    void Put(T t) {
        if (this->size >= this->cap) {
            this->cap *= 1.5;
            T *nl = NEWN(T, this->cap);
            memcpy(nl, this->list, this->size * sizeof(T));
            DELETEN(T, this->cap, this->list);
            this->list = nl;
        }

        this->list[this->size++] = t;
    }

    T Get(int i) {
        return this->list[i];
    }

private:
    T *list;
    int size;
    int cap;
};


//hash map
KHASH_MAP_INIT_INT64(VOID64, void*);

template<typename T>
class KMap {
public:
    KMap() {
        this->map = kh_init(VOID64);
    }

    ~KMap() {
        kh_clear(VOID64, this->map);
        kh_destroy(VOID64, this->map);
    }

    void Clear() {
        kh_clear(VOID64, this->map);
    }

    void ClearData() {
        for (khiter_t k = kh_begin(this->map); k != kh_end(this->map); ++k) {
            if (kh_exist(this->map, k)) {
                T *v = (T*)kh_value(this->map, k);
                DEL(T, v);
            }
        }
        this->Clear();
    }

    int Size() {
        return kh_size(this->map);
    }

    void Put(uint64_t key, T *t) {
        khiter_t k;
        int ret = 0;
        k = kh_put(VOID64, this->map, key, &ret);
        kh_value(this->map, k) = t;
    }

    T* Find(uint64_t key) {
        khiter_t k = kh_get(VOID64, this->map, key);
        if (k != kh_end(this->map)) {
            T* v = (T*)kh_value(this->map, k);
            return v;
        }

        return NULL;
    }

    bool Remove(uint64_t key) {
        khiter_t k = kh_get(VOID64, this->map, key);
        if (k != kh_end(this->map)) {
            kh_del(VOID64, this->map, k);
            return true;
        }

        return false;
    }

#define KMAP_FOREACH_KV(m, t, v, key) for (khiter_t k = kh_begin((m)->map); k != kh_end((m)->map); ++k) { \
        if (kh_exist((m)->map, k)) {                                    \
            t *v = (t*)kh_value((m)->map, k);                            \
            uint64_t key = kh_key((m)->map, k);                           \

#define KMAP_FOREACH_V(m, t, v) for (khiter_t k = kh_begin((m)->map); k != kh_end((m)->map); ++k) { \
        if (kh_exist((m)->map, k)) {                                    \
            t *v = (t*)kh_value((m)->map, k);                            \

#define KMAP_FOREACH_END() } } 

public:
    khash_t(VOID64) *map;
};



#define BM_MARK_FALSE 0
#define BM_MARK_TRUE  1
#define BM_MARK_ERROR 2

class BitMarker {
public:
    BitMarker(int bit_size);
    ~BitMarker();

    int Mark(uint32_t idx);
    int ClearMark(uint32_t idx);
    int IsMark(uint32_t idx);
    void Reset(uint32_t idx);

    bool AdjustStartPosition(uint32_t pos);
private:
    char *buf;
    uint32_t bit_size;
    uint32_t byte_size;

    uint32_t start;
};

class Buffer {
public:
    Buffer(size_t size)
        : size_(size), buf_(MALLOC(size))
    {}
    ~Buffer() {
        FREE(buf_);
    }
    size_t size() const { return size_; }
    operator char * () const { return (char *)buf_; }
private:
    Buffer(const Buffer &);
    Buffer & operator = (const Buffer &);
    size_t const size_;
    void * const buf_;
};

}

#endif//__QPP_UTILS_H__
