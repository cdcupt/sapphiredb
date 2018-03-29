#ifndef SAPPHIREDB_COMMON_NET_H_
#define SAPPHIREDB_COMMON_NET_H_

#include <string>

namespace sapphiredb
{
namespace common
{
class Netcon{
protected:
    enum NetType{
        IPV4 = 1,
        IPV6 = 2
    };

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
        inline Data(uint32_t size = 1000){
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
    Netcon(::std::string addr, uint32_t port, NetType type, uint32_t bufsize);
    ~Netcon();

    virtual void send() = 0;
    virtual void recv() = 0;
    virtual void loop_once(uint32_t waitms) = 0;

    bool empty();
protected:
    Data* recvbuf;
};
}
}

#endif
