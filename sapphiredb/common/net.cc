#include "common/net.h"

sapphiredb::common::Netcon::Data* sapphiredb::common::Netcon::getData(){
    return recvbuf;
}
bool sapphiredb::common::Netcon::pushData(::std::string& data){
    if(sendbuf->len > 0) return false;
    else if(sendbuf->len+data.size() > sendbuf->size) return false;
    else{
        ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
        if(sendbuf->len+data.size() <= sendbuf->size){
            int j = this->sendbuf->len;
            for(auto ch : data){
                (*(this->sendbuf->buf))[j++] = ch;
                ++sendbuf->len;
            }
            (*(this->sendbuf->buf))[j] = '\0';
        }
    }
    return true;
}
bool sapphiredb::common::Netcon::pushData(::std::string&& data){
    if(sendbuf->len > 0) return false;
    else if(sendbuf->len+data.size() > sendbuf->size) return false;
    else{
        ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
        if(sendbuf->len+data.size() <= sendbuf->size){
            int j = this->sendbuf->len;
            for(auto ch : data){
                (*(this->sendbuf->buf))[j++] = ch;
                ++sendbuf->len;
            }
            (*(this->sendbuf->buf))[j] = '\0';
        }
    }
    return true;
}
::std::string sapphiredb::common::Netcon::popData(){
    if(recvbuf->len <= 0) return "";
    else{
        ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
        if(recvbuf->len > 0){
            uint32_t slen = recvbuf->len;
            recvbuf->len = 0;
            return ::std::string(this->recvbuf->buf->begin(), this->recvbuf->buf->begin()+slen);
        }
    }

    return "";
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

void sapphiredb::common::Netcon::pushUnknownfd(int32_t& fd){
    std::unique_lock<std::mutex> lock(this->unknownfd_mutex);
    unknownfd.push(fd);
}

void sapphiredb::common::Netcon::pushUnknownfd(int32_t&& fd){
    std::unique_lock<std::mutex> lock(this->unknownfd_mutex);
    unknownfd.push(fd);
}

int32_t sapphiredb::common::Netcon::popUnknownfd(){
    std::unique_lock<std::mutex> lock(this->unknownfd_mutex);
    int32_t fd = unknownfd.front();
    unknownfd.pop();
    return fd;
}

bool sapphiredb::common::Netcon::emptyUnknownfd(){
    return this->unknownfd.empty();
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

