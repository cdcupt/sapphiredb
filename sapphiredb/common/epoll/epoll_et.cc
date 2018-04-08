#include "common/epoll_et.h"

void sapphiredb::common::Epoll::setNonBlock(int32_t fd) {
    int32_t flags = fcntl(fd, F_GETFL, 0);
    exit_if(flags<0, "fcntl failed");
    int32_t r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    exit_if(r<0, "fcntl failed");
}

void sapphiredb::common::Epoll::updateEvents(int32_t efd, int32_t fd, int32_t events, int32_t op) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    printf("%s fd %d events read %d write %d\n",
           op==EPOLL_CTL_MOD?"mod":"add", fd, ev.events & EPOLLIN, ev.events & EPOLLOUT);
    int32_t r = epoll_ctl(efd, op, fd, &ev);
    exit_if(r, "epoll_ctl failed");
}

void sapphiredb::common::Epoll::handleAccept(int32_t efd, int32_t fd) {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int32_t cfd = accept(fd,(struct sockaddr *)&raddr,&rsz);
    exit_if(cfd<0, "accept failed");
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int32_t r = getpeername(cfd, (sockaddr*)&peer, &alen);
    exit_if(r<0, "getpeername failed");
    printf("accept a connection from %s\n", inet_ntoa(raddr.sin_addr));
    setNonBlock(cfd);
    updateEvents(efd, cfd, EPOLLIN|EPOLLOUT|EPOLLET, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::handleRead(int32_t efd, int32_t fd) {
    char buf[4096];
    int32_t n = 0;
    for(;;){
        while((n=::read(fd, buf, sizeof(buf))) > 0){
            if(n+this->recvbuf->size > this->recvbuf->len){
                //TODO error log
                ::std::cerr << "buf is really fill!" << ::std::endl;
                return;
            }
            else{
                this->recvbuf->buf->append(static_cast<::std::strin>(buf));
            }
        }
        if(n == 0){
            //TODO error log
            ::std::cerr << "socket closed!" << ::std::endl;
            return;
        }
        if(n<0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return;
        if(n<0 && (errno == EINTR)){
            continue;
        }
        exit_if(n<0, "read error");
    }
    updateEvents(efd, fd, EPOLLIN, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::handleWrite(int32_t efd, int32_t fd) {
    std::lock_guard<std::mutex> guard(this->buf_mutex);
    ::std::string buf = ::std::move(*(this->recvbuf->buf));
    for(;;){
        int32_t r = ::write(fd, buf.c_str(), buf.size()); //写出读取的数据
        if(r<0 && (errno == EAGAIN || errno == EINTR)){
            continue;
        }
        else if(r == 0){
            //TODO error log
            ::std::cerr << "socket closed!" << ::std::endl;
            return;
        }
        else if(r < 0){
            ::std::cerr << "write error" << ::std::endl;
        }
    }
    //实际应用中，写出数据可能会返回EAGAIN，此时应当监听可写事件，当可写时再把数据写出
    //exit_if(r<=0, "write error");
    //实际应用应当实现可写时写出数据，无数据可写才关闭可写事件
    updateEvents(efd, fd, EPOLLIN, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::epoll_loop_once(int32_t efd, int32_t lfd, int32_t waitms) {
    const int32_t kMaxEvents = 20;
    struct epoll_event activeEvs[100];
    int32_t n = epoll_wait(efd, activeEvs, kMaxEvents, waitms);
    for (int32_t i = 0; i < n; i ++) {
        int32_t fd = activeEvs[i].data.fd;
        int32_t events = activeEvs[i].events;
        if (events & (EPOLLIN | EPOLLERR)) {
            if (fd == lfd) {
                handleAccept(efd, fd);
            } else {
                handleRead(efd, fd);
            }
        } else if (events & EPOLLOUT) {
            handleWrite(efd, fd);
        } else {
            exit_if(1, "unknown event");
        }
    }
}

void sapphiredb::common::Epoll::send(){
    if(!this->empty()){
        updateEvents(this->epollfd, this->listenfd, EPOLLOUT, EPOLL_CTL_ADD);
    }
}

void sapphiredb::common::Epoll::recv(){
    updateEvents(this->epollfd, this->listenfd, EPOLLIN, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::loop_once(uint32_t waitms){
    epoll_loop_once(this->epollfd, this->listenfd, waitms);
}

sapphiredb::common::Epoll::Epoll(::std::string addr, uint32_t port, NetType type, uint32_t bufsize, uint32_t fdsize) 
    : Netcon(addr, port, type, bufsize){
    ::signal(SIGPIPE, SIG_IGN);
    try{
        this->epollfd = epoll_create(fdsize);
        this->listenfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        this->rbind = ::bind(listenfd,(struct sockaddr *)&addr, sizeof(struct sockaddr));
        this->rbind = listen(listenfd, listenq);
        this->setNonBlock(this->listenfd);
        this->updateEvents(epollfd, listenfd, EPOLLIN, EPOLL_CTL_ADD);
    }
    catch(...){
        ::std::cerr << "epoll alloc fd error" << ::std::endl;
    }
}

sapphiredb::common::Epoll::~Epoll(){
    close(this->epollfd);
}