#ifndef SAPPHIREDB_COMMON_KQUEUE_H_
#define SAPPHIREDB_COMMON_KQUEUE_H_

#include <string>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "common/net.h"

#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("error no: %d error msg %s\n", errno, strerror(errno)); exit(1);}

namespace sapphiredb
{
namespace common
{
class Kqueue : public Netcon{
private:
    enum Event{
        KQUEUE_READ_EVENT = 1,
        KQUEUE_WRITE_EVENT = 2
    };

    enum RESTATE{
        REUSE = 1,
        NOREUSE = 2,
        CLOSED = 3,
        FILLED = 4,
        UNKNOW = 5
    };

    int32_t epollfd;
    int32_t listenfd;
    struct sockaddr_in sockaddr;
    int32_t rbind;
    ::std::mutex buf_mutex;

    void setNonBlock(int32_t fd);
    void updateEvents(int32_t efd, int32_t fd, int32_t events, bool modify);
    void delete_event(int32_t efd, int32_t fd, int32_t events);
    void handleAccept(int32_t efd, int32_t fd);
    void handleRead(int32_t efd, int32_t fd);
    void handleWrite(int32_t efd, int32_t fd);
    void kqueue_loop_once(int32_t efd, int32_t lfd, int32_t waitms);
public:
    Kqueue();
    ~Kqueue();

    virtual void send() override;
    virtual void recv() override;
    virtual void loop_once(uint32_t waitms) override;
};
}
}

#endif
