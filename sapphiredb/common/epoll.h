#ifndef SAPPHIREDB_COMMON_EPOLL_H_
#define SAPPHIREDB_COMMON_EPOLL_H_

#include <string>
#include <iostream>
#include <mutex>
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
#include <vector>
#include <set>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "common/net.h"
#include "common/spdlog/include/spdlog/spdlog.h"

namespace sapphiredb
{
namespace common
{
class Epoll : public Netcon{
private:
    #define exit_if(r, ...) if(r) {logger->error(__VA_ARGS__); logger->error("error no: {:d} error msg {:s}", errno, strerror(errno)); exit(1);}

    enum RESTATE{
        REUSE = 1,
        NOREUSE = 2,
        CLOSED = 3,
        FILLED = 4,
        UNKNOW = 5
    };

    int32_t epollfd;
    int32_t listenfd;
    ::std::unordered_map<uint64_t, int32_t> peersfd;
    ::std::queue<int32_t> readfd;
    ::std::vector<int32_t> bcastfd;
    int32_t rbind;
    ::std::mutex buf_mutex;
    ::std::mutex queue_mutex;
    std::shared_ptr<spdlog::logger> logger;

    ::std::condition_variable* node_send_condition;
    ::std::condition_variable* node_recv_condition;
    ::std::condition_variable* node_bind_condition;
    ::std::condition_variable* node_step_condition;

    void setNonBlock(int32_t fd);
    void sapphiredb::common::Epoll::updateEvents(int32_t efd, int32_t fd, int32_t events, int32_t op);
    void connecttopeer(::std::string&& ip, uint32_t port, uint64_t id);
    void handleAccept(int32_t efd, int32_t fd);
    void handleRead(int32_t efd, int32_t fd);
    void handleWrite(int32_t efd, int32_t fd);
    void handleConnect(int32_t efd, int32_t fd);
    void epoll_loop_once(int32_t efd, int32_t lfd, int32_t waitms);
public:
    Epoll(::std::string ip, uint32_t port, NetType type, uint32_t bufsize, uint32_t fdsize, uint32_t listenq,
        ::std::condition_variable* tsend_condition = nullptr, ::std::condition_variable* trecv_condition = nullptr,
        ::std::condition_variable* tbind_condition = nullptr, ::std::condition_variable* tstep_condition = nullptr);
    virtual ~Epoll() override;

    //interface
    virtual void send(uint64_t id) override; //p2p send
    virtual void recv(uint64_t id) override; //p2p recieve
    virtual void conn(::std::string&& ip, uint32_t port, uint64_t id) override; //p2p connect
    virtual void listenp(uint32_t listenq = 20) override;
    virtual void loop_once(uint32_t waitms) override; //epoll loop
    void doSomething(std::function<void(int32_t fd)> task); //after read callback
    void bindPeerfd(uint64_t, int32_t);
    void funcPeerfd(std::function<void(::std::unordered_map<uint64_t, int32_t>&)> func);

    inline bool isRecvEmpty(){
        return this->recvbuf->len <= 0;
    }

    inline bool isSendEmpty(){
        return this->sendbuf->len <= 0;
    }
};
} // namespace common
} // namespace sapphiredb

#endif
