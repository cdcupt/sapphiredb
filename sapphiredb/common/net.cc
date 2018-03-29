#include "common/net.h"

sapphiredb::common::Netcon::Netcon(::std::string addr, uint32_t port, NetType type, uint32_t bufsize){
    recvbuf = new Data(bufsize);
    if(type == sapphiredb::common::Netcon::IPV4){
        this->_addr_ipv4 = addr;
        this->_port = port;
    }
}

bool sapphiredb::common::Netcon::empty(){
    return this->recvbuf->buf->empty();
}
