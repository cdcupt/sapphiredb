#ifndef SAPPHIREDB_COMMON_EPOLL_ET_H_
#define SAPPHIREDB_COMMON_EPOLL_ET_H_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <signal.h>
#include "common/net.h"

#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("%s:%d error no: %d error msg %s\n", __FILE__, __LINE__, errno, strerror(errno)); exit(1);}

namespace sapphiredb
{
namespace common
{
class Epoll : public Netconn{
private:
    int32_t epollfd;
    int32_t listenfd;
    int32_t rbind;
    ::std::mutex buf_mutex;

    void setNonBlock(int32_t fd);
    void updateEvents(int32_t efd, int32_t fd, int32_t events, int32_t op);
    void handleAccept(int32_t efd, int32_t fd);
    void handleRead(int32_t efd, int32_t fd);
    void handleWrite(int32_t efd, int32_t fd);
    void epoll_loop_once(int32_t efd, int32_t lfd, int32_t waitms);
public:
    Epoll(::std::string addr, uint32_t port, NetType type, uint32_t bufsize, uint32_t fdsize, uint32_t listenq);
    virtual ~Epoll();

    virtual void send() override;
    virtual void recv() override;
    virtual void loop_once(uint32_t waitms) override;
};
} // namespace common
} // namespace sapphiredb

#endif
