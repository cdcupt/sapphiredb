#include "common/net.h"

sapphiredb::common::Netcon::Data* sapphiredb::common::Netcon::getData(){
    return recvbuf;
}
bool sapphiredb::common::Netcon::stackingData(::std::string& data){
    if(sendbuf->len+data.size() > sendbuf->size) return false;
    else{
        ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
        int j = this->sendbuf->len;
        for(auto ch : data){
            (*(this->sendbuf->buf))[j++] = ch;
        }
        sendbuf->len += data.size();
    }
    return true;
}
void sapphiredb::common::Netcon::clearSendbuf(){
    ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
    sendbuf->buf->clear();
    sendbuf->len = 0;
}
void sapphiredb::common::Netcon::clearRecvbuf(){
    ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
    recvbuf->buf->clear();
    recvbuf->len = 0;
}

sapphiredb::common::Netcon::Netcon(::std::string addr, uint32_t port, NetType type, uint32_t bufsize){
    recvbuf = new Data(bufsize);
    sendbuf = new Data(bufsize);
    if(type == sapphiredb::common::Netcon::IPV4){
        this->_addr_ipv4 = addr;
        this->_port = port;
    }
}
sapphiredb::common::Netcon::~Netcon(){
    delete sendbuf;
    delete recvbuf;
}

