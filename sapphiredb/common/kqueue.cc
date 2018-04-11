#include "common/kqueue.h"

void sapphiredb::common::Kqueue::setNonBlock(int32_t fd) {
    int32_t flags = fcntl(fd, F_GETFL, 0);
    exit_if(flags < 0, "fcntl failed");
    int32_t r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    exit_if(r<0, "fcntl failed");
}

void sapphiredb::common::Kqueue::updateEvents(int32_t efd, int32_t fd, int32_t events, bool modify) {
    struct kevent ev[2];
    int32_t n = 0;
    if (events & KQUEUE_READ_EVENT) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)fd);
    } else if (modify){
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    }
    if (events & KQUEUE_WRITE_EVENT) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)fd);
    } else if (modify){
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    }
    logger->info("update fd[{:d}] into kqueue", fd);
    int32_t r = kevent(efd, ev, n, NULL, 0, NULL);
    exit_if(r, "kevent failed ");
}

void sapphiredb::common::Kqueue::delete_event(int32_t efd, int32_t fd, int32_t events) {
    struct kevent ev[2];
    int32_t n = 0;
    if (events & KQUEUE_READ_EVENT) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    }
    if (events & KQUEUE_WRITE_EVENT) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    }
    logger->info("delete fd[{:d}] into kqueue", fd);
    int32_t r = kevent(efd, ev, n, NULL, 0, NULL);
    exit_if(r, "kevent failed ");
}

void sapphiredb::common::Kqueue::connecttopeer(::std::string&& ip, uint32_t port, uint64_t id) {
    struct sockaddr_in servaddr;
    int32_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
    bind(sockfd,(struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(id == 0) unknownfd.insert(sockfd);
    else peersfd[id] = sockfd;
    handleConnect(this->epollfd, sockfd);
}

void sapphiredb::common::Kqueue::handleAccept(int32_t efd, int32_t fd) {
    //::std::cout << "****handleAccept****" << ::std::endl;
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int32_t cfd = accept(fd,(struct sockaddr *)&raddr,&rsz);
    exit_if(cfd<0, "accept failed");
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int32_t r = getpeername(cfd, reinterpret_cast<struct sockaddr *>(&peer), &alen);
    exit_if(r<0, "getpeername failed");
    logger->info("accept a connection from {:s}", inet_ntoa(raddr.sin_addr));
    unknownfd.insert(cfd);
    setNonBlock(cfd);
    updateEvents(efd, cfd, KQUEUE_READ_EVENT|KQUEUE_WRITE_EVENT, false);
}

void sapphiredb::common::Kqueue::handleRead(int32_t efd, int32_t fd) {
    //::std::cout << "****handleRead****" << ::std::endl;
    char buf[4096];
    int32_t n = 0;
    for(;;){
        while ((n=::read(fd, buf, sizeof(buf))) > 0) {
            if(n+this->recvbuf->len > this->recvbuf->size){
                logger->warn("buf is really fill when read fd[{:d}]", fd);
                return;
            }
            else{
                ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
                int j = this->recvbuf->len;
                for(int i=0; buf[i]!='\0'; ++i){
                    (*(this->recvbuf->buf))[j++] = buf[i];
                }
                this->recvbuf->len += n;
            }
        }
        
        if(n == 0){
            delete_event(efd, fd, KQUEUE_READ_EVENT);
            logger->warn("fd[{:d}] socket closed!", fd);
            return;
        }
        
        if(n<0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            logger->warn("Read fd[{:d}] EAGAIN", fd);
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->readfd.emplace(fd);
            return;
        }
        if(n<0 && (errno == EINTR)){
            continue;
        }
        exit_if(n<0, "read error");
    }
}

void sapphiredb::common::Kqueue::handleWrite(int32_t efd, int32_t fd) {
    //::std::cout << "****handleWrite****" << ::std::endl;
    ::std::lock_guard<std::mutex> guard(this->buf_mutex);
    ::std::string buf = ::std::move(*(this->sendbuf->buf));
    for(;;){
        
        int32_t r = ::write(fd, buf.c_str(), this->sendbuf->len); //写出读取的数据
        if(r<0 && (errno == EAGAIN || errno == EINTR)){
            logger->warn("Write fd[{:d}] EAGAIN OR EINTR?", fd);
            continue;
        }
        else if(r == 0){
            delete_event(efd, fd, KQUEUE_WRITE_EVENT);
            logger->warn("fd[{:d}] socket closed!", fd);
            return;
        }
        else if(r < 0){
            delete_event(efd, fd, KQUEUE_WRITE_EVENT);
            logger->error("Write fd[{:d}] write error", fd);
            return;
        }
        else{
            this->sendbuf->len -= r;
            if(this->sendbuf->len == 0) break;
        }
    }
    delete_event(efd, fd, KQUEUE_WRITE_EVENT);
}

void sapphiredb::common::Kqueue::handleConnect(int32_t efd, int32_t fd) {
    //::std::cout << "****handleConnect****" << ::std::endl;
    setNonBlock(fd);
    updateEvents(efd, fd, KQUEUE_READ_EVENT, false);
}

void sapphiredb::common::Kqueue::doSomething(std::function<void(int32_t fd)> task){
    if(!this->readfd.empty()){
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        if(!this->readfd.empty()){
            int32_t fd = std::move(this->readfd.front());
            this->readfd.pop();
            task(fd);
            close(fd);
            clearRecvbuf();
        }
    }
}

void sapphiredb::common::Kqueue::kqueue_loop_once(int32_t efd, int32_t lfd, int32_t waitms) {
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;
    const int32_t kMaxEvents = 20;
    struct kevent activeEvs[kMaxEvents];
    int32_t n = kevent(efd, NULL, 0, activeEvs, kMaxEvents, &timeout);
    for (int32_t i = 0; i < n; i ++) {
        int32_t fd = (int32_t)(intptr_t)activeEvs[i].udata;
        int32_t events = activeEvs[i].filter;
        if (events == EVFILT_READ) {
            if (fd == lfd) {
                handleAccept(efd, fd);
            } else {
                handleRead(efd, fd);
            }
        } else if (events == EVFILT_WRITE) {
            handleWrite(efd, fd);
        } else {
            exit_if(1, "unknown event");
        }
    }
}

void sapphiredb::common::Kqueue::send(uint64_t id){
    if(this->sendbuf->len > 0){
        if(id == 0){
            for(auto ufd : unknownfd){
                updateEvents(this->epollfd, ufd, KQUEUE_WRITE_EVENT, false);
            }
        }
        else{
            updateEvents(this->epollfd, peersfd[id], KQUEUE_WRITE_EVENT, false);
        }
    }
}

void sapphiredb::common::Kqueue::recv(uint64_t id){
    if(id == 0){
        for(auto ufd : unknownfd){
            updateEvents(this->epollfd, ufd, KQUEUE_READ_EVENT, false);
        }
    }
    else{
        updateEvents(this->epollfd, peersfd[id], KQUEUE_READ_EVENT, false);
    }
}

void sapphiredb::common::Kqueue::conn(::std::string&& ip, uint32_t port, uint64_t id){
    connecttopeer(std::forward<::std::string>(ip), port, id);
}

void sapphiredb::common::Kqueue::loop_once(uint32_t waitms){
    kqueue_loop_once(this->epollfd, this->listenfd, waitms);
}

void sapphiredb::common::Kqueue::listenp(uint32_t listenq){
    listen(this->listenfd, listenq);
    this->setNonBlock(this->listenfd);
    this->updateEvents(this->epollfd, this->listenfd, KQUEUE_READ_EVENT, false);
}

sapphiredb::common::Kqueue::Kqueue(::std::string ip, uint32_t port, NetType type, uint32_t bufsize, uint32_t fdsize, uint32_t listenq)
    : Netcon(ip, port, type, bufsize){
    ::signal(SIGPIPE, SIG_IGN);
    try{
        this->epollfd = kqueue();
        this->listenfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        int32_t rbind = bind(listenfd,(struct sockaddr *)&addr, sizeof(struct sockaddr));
        if(rbind) throw "bind error";

        //this->logger = spdlog::basic_logger_mt("logger", "kqueue_log.txt"); 
        this->logger = spdlog::stdout_color_mt("console");
        //this->logger = spdlog::rotating_logger_mt("logger", "kqueue_log.txt", 1048576 * 5, 3);
    }
    catch(...){
        ::std::cerr << "epoll alloc fd error" << ::std::endl;
    }
}

sapphiredb::common::Kqueue::~Kqueue(){
    close(this->listenfd);
    close(this->epollfd);
    for(auto peerfd : this->peersfd){
        close(peerfd.second);
    }
    for(auto ufd : this->unknownfd){
        close(ufd);
    }
    while(!this->readfd.empty()){
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        while(!this->readfd.empty()){
            int32_t fd = std::move(this->readfd.front());
            this->readfd.pop();
            close(fd);
        }
    }
    try{
        spdlog::drop_all();
    }
    catch(...){
        ::std::abort();
    }
}
