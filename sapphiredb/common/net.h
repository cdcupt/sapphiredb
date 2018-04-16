#ifndef SAPPHIREDB_COMMON_NET_H_
#define SAPPHIREDB_COMMON_NET_H_

#include <string>
#include <iostream>
#include <mutex>
#include <utility>

namespace sapphiredb
{
namespace common
{
class Netcon{
private:
    ::std::mutex buf_mutex;
protected:
    class Data{
    public:
        enum MessageType{
            Massage = 1,
            Log = 2,
            Snapshot = 3,
            Unknow = 4
        };
        uint32_t len;
        uint32_t size;
        ::std::string* buf;
        MessageType type;
    public:
        inline Data(uint32_t s = 1000) : len(0), size(s){
            buf = new ::std::string(size, '\0');
        }
        inline ~Data(){
            delete buf;
        }
    };

    uint32_t _port;
    ::std::string _addr_ipv4;
    ::std::string _addr_ipv6;
public:
    enum NetType{
        IPV4 = 1,
        IPV6 = 2
    };

    Netcon(::std::string addr, uint32_t port, NetType type, uint32_t bufsize);
    virtual ~Netcon();

    virtual void send(uint64_t id) = 0;
    virtual void recv(uint64_t id) = 0;
    virtual void conn(::std::string&& ip, uint32_t port, uint64_t id) = 0;
    virtual void listenp(uint32_t listenq = 20) = 0;
    virtual void loop_once(uint32_t waitms) = 0;

    Data* getData();
    bool pushData(::std::string& data);
    bool pushData(::std::string&& data);
    ::std::string popData();
    void clearSendbuf();
    void clearRecvbuf();
protected:
    Data* recvbuf;
    Data* sendbuf;
};
}
}

#endif
