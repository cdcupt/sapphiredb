#include "common/epoll.h"

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
    //printf("%s fd %d events read %d write %d\n",
    //       op==EPOLL_CTL_MOD?"mod":"add", fd, ev.events & EPOLLIN, ev.events & EPOLLOUT);
    int32_t r = epoll_ctl(efd, op, fd, &ev);
    exit_if(r, "epoll_ctl failed");
}

void sapphiredb::common::Epoll::connecttopeer(::std::string&& ip, uint32_t port, uint64_t id) {
    struct sockaddr_in servaddr;
    int32_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
    bind(sockfd,(struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(id == 0) {
        logger->error("Do not support broadcast!");
        //::std::cout << "sockfd: " << sockfd << ::std::endl;
        bcastfd.push_back(sockfd);
    }
    else {
        peersfd[id] = sockfd;
        ::std::cout << "id: " << id << " ## peersfd: " << peersfd[id] << ::std::endl;
    }
    handleConnect(this->epollfd, sockfd);
}

void sapphiredb::common::Epoll::handleAccept(int32_t efd, int32_t fd) {
    //::std::cout << "****handleAccept****" << ::std::endl;
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int32_t cfd = accept(fd,(struct sockaddr *)&raddr,&rsz);
    exit_if(cfd<0, "accept failed");
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int32_t r = getpeername(cfd, reinterpret_cast<struct sockaddr *>(&peer), &alen);
    exit_if(r<0, "getpeername failed");
    logger->error("accept a connection from {:s} add got fd[{:d}]", inet_ntoa(raddr.sin_addr), cfd);
    pushUnknownfd(cfd);
    this->node_bind_condition->notify_all();
    bcastfd.push_back(cfd);
    setNonBlock(cfd);
    updateEvents(efd, cfd, EPOLLIN|EPOLLET, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::handleRead(int32_t efd, int32_t fd) {
    //::std::cout << "****handleRead****" << ::std::endl;
    char buf[4096];
    int32_t n = 0;
    for(;;){
        while ((n=::read(fd, buf, sizeof(buf))) > 0) {
            ::std::lock_guard<::std::mutex> lock(this->buf_mutex);
            if(n+this->recvbuf->len > this->recvbuf->size){
                //updateEvents(efd, fd, EPOLLIN, EPOLL_CTL_DEL);
                logger->warn("buf is really fill when read fd[{:d}]", fd);
                return;
            }
            else{
                int j = this->recvbuf->len;
                for(int i=0; buf[i]!='\0'; ++i){
                    (*(this->recvbuf->buf))[j++] = buf[i];
                }
                //(*(this->recvbuf->buf))[j] = '\0';
                this->recvbuf->len += n;
            }
        }
        
        if(n == 0){
            updateEvents(efd, fd, EPOLLIN, EPOLL_CTL_DEL);
            for(::std::unordered_map<uint64_t, int32_t>::iterator peer = peersfd.begin(); peer != peersfd.end(); ++peer){
                if(peer->second == fd){
                    peersfd.erase(peer);
                    break;
                }
            }
            logger->warn("fd[{:d}] socket closed!", fd);
            return;
        }
        
        if(n<0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            //logger->warn("Read fd[{:d}] EAGAIN", fd);
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->readfd.emplace(fd);
            this->node_recv_condition->notify_all();
            return;
        }
        if(n<0 && (errno == EINTR)){
            continue;
        }
        exit_if(n<0, "read error");
    }
}

void sapphiredb::common::Epoll::handleWrite(int32_t efd, int32_t fd) {
    //::std::cout << "****handleWrite****" << ::std::endl;
    ::std::lock_guard<std::mutex> guard(this->buf_mutex);
    ::std::string buf = ::std::string(this->sendbuf->buf->begin(), this->sendbuf->buf->begin()+this->sendbuf->len);
    //::std::string buf = ::std::move(*(this->sendbuf->buf)); // terrible code
    for(;;){
        int32_t r = ::write(fd, buf.c_str(), this->sendbuf->len); //写出读取的数据
        if(r<0 && (errno == EAGAIN || errno == EINTR)){
            //logger->warn("Write fd[{:d}] EAGAIN OR EINTR?", fd);
            continue;
        }
        else if(r == 0){
            updateEvents(this->epollfd, fd, EPOLLIN|EPOLLET, EPOLL_CTL_MOD);
            logger->warn("fd[{:d}] socket closed!", fd);
            return;
        }
        else if(r < 0){
            updateEvents(this->epollfd, fd, EPOLLIN|EPOLLET, EPOLL_CTL_MOD);
            logger->error("Write fd[{:d}] write error", fd);
            return;
        }
        else{
            this->sendbuf->len -= r;
            if(this->sendbuf->len <= 0) break;
        }
    }
    updateEvents(this->epollfd, fd, EPOLLIN|EPOLLET, EPOLL_CTL_MOD);
}

void sapphiredb::common::Epoll::handleConnect(int32_t efd, int32_t fd) {
    //::std::cout << "****handleConnect****" << ::std::endl;
    setNonBlock(fd);
    updateEvents(efd, fd, EPOLLIN|EPOLLET, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::doSomething(std::function<void(int32_t fd)> task){
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

void sapphiredb::common::Epoll::send(uint64_t id){
    if(this->sendbuf->len > 0){
        if(id == 0){
            for(auto ufd : bcastfd){
                updateEvents(this->epollfd, ufd, EPOLLIN|EPOLLOUT|EPOLLET, EPOLL_CTL_MOD);
            }
        }
        else{
            if(peersfd.find(id) == peersfd.end() || peersfd[id] == 0){
                this->sendbuf->len = 0;
                return;
            }
            updateEvents(this->epollfd, peersfd[id], EPOLLIN|EPOLLOUT|EPOLLET, EPOLL_CTL_MOD);
        }
    }
}

void sapphiredb::common::Epoll::recv(uint64_t id){
    if(id == 0){
        for(auto ufd : bcastfd){
            updateEvents(this->epollfd, ufd, EPOLLIN|EPOLLET, EPOLL_CTL_ADD);
        }
    }
    else{
        //if(peersfd.find(id) == peersfd.end() || peersfd[id] == 0) return;
        updateEvents(this->epollfd, peersfd[id], EPOLLIN|EPOLLET, EPOLL_CTL_ADD);
    }
}

void sapphiredb::common::Epoll::conn(::std::string&& ip, uint32_t port, uint64_t id){
    connecttopeer(std::forward<::std::string>(ip), port, id);
}

void sapphiredb::common::Epoll::loop_once(uint32_t waitms){
    epoll_loop_once(this->epollfd, this->listenfd, waitms);
}

void sapphiredb::common::Epoll::listenp(uint32_t listenq){
    listen(this->listenfd, listenq);
    this->setNonBlock(this->listenfd);
    updateEvents(this->epollfd, this->listenfd, EPOLLIN|EPOLLET, EPOLL_CTL_ADD);
}

void sapphiredb::common::Epoll::bindPeerfd(uint64_t id, int32_t fd){
    if(peersfd.find(id) == peersfd.end()){
        peersfd[id] = fd;
    }
    else{
        //TODO something else
        logger->warn("id[{:d}] restart!", id);
        peersfd[id] = fd;
    }
}

void sapphiredb::common::Epoll::funcPeerfd(std::function<void(::std::unordered_map<uint64_t, int32_t>& peersfd)> func){
    func(this->peersfd);
}

sapphiredb::common::Epoll::Epoll(::std::string ip, uint32_t port, NetType type, uint32_t bufsize, uint32_t fdsize, uint32_t listenq,
    ::std::condition_variable* tsend_condition, ::std::condition_variable* trecv_condition,
    ::std::condition_variable* tbind_condition, ::std::condition_variable* tstep_condition)
    : Netcon(ip, port, type, bufsize){
    ::signal(SIGPIPE, SIG_IGN);
    try{
        this->epollfd = epoll_create(fdsize);;
        this->listenfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        int32_t rbind = bind(listenfd,(struct sockaddr *)&addr, sizeof(struct sockaddr));
        if(rbind) throw "bind error";

        //this->logger = spdlog::basic_logger_mt("logger", "epoll_log.txt"); 
        this->logger = spdlog::stdout_color_mt("epoll_console");
        //this->logger = spdlog::rotating_logger_mt("logger", "epoll_log.txt", 1048576 * 5, 3);

        this->node_send_condition = tsend_condition;
        this->node_recv_condition = trecv_condition;
        this->node_bind_condition = tbind_condition;
        this->node_step_condition = tstep_condition;
    }
    catch(...){
        ::std::cerr << "epoll alloc fd error" << ::std::endl;
    }
}

sapphiredb::common::Epoll::~Epoll(){
    close(this->listenfd);
    close(this->epollfd);
    for(auto peerfd : this->peersfd){
        close(peerfd.second);
    }
    for(auto fd : this->bcastfd){
        close(fd);
    }
    while(!this->unknownfd.empty()){
        close(this->unknownfd.front());
        this->unknownfd.pop();
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
