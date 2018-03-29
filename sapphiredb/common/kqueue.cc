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
    //TODO log
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
    //TODO log
    int32_t r = kevent(efd, ev, n, NULL, 0, NULL);
    exit_if(r, "kevent failed ");
}

void sapphiredb::common::Kqueue::handleAccept(int32_t efd, int32_t fd) {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int32_t cfd = accept(fd,(struct sockaddr *)&raddr,&rsz);
    exit_if(cfd<0, "accept failed");
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int32_t r = getpeername(cfd, reinterpret_cast<struct sockaddr *>(&peer), &alen);
    exit_if(r<0, "getpeername failed");
    printf("accept a connection from %s\n", inet_ntoa(raddr.sin_addr));
    setNonBlock(cfd);
    updateEvents(efd, cfd, KQUEUE_READ_EVENT|KQUEUE_WRITE_EVENT, false);
}

void sapphiredb::common::Kqueue::handleRead(int32_t efd, int32_t fd) {
    char buf[4096];
    int32_t n = 0;
    for(;;){
        while ((n=::read(fd, buf, sizeof buf)) > 0) {
            if(n+this->recvbuf->size > this->recvbuf->len){
                //TODO error log
                ::std::cerr << "buf is really fill!" << ::std::endl;
                return;
            }
            else{
                this->recvbuf->buf->append(static_cast<::std::string>(buf));
            }
            //int32_t r = ::write(fd, buf, n); //写出读取的数据
            //实际应用中，写出数据可能会返回EAGAIN，此时应当监听可写事件，当可写时再把数据写出
            //exit_if(r<=0, "write error");
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
}

void sapphiredb::common::Kqueue::handleWrite(int32_t efd, int32_t fd) {
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
    updateEvents(efd, fd, KQUEUE_READ_EVENT, true);
}

void sapphiredb::common::Kqueue::kqueue_loop_once(int32_t efd, int32_t lfd, int32_t waitms) {
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;
    const int32_t kMaxEvents = 20;
    struct kevent activeEvs[kMaxEvents];
    int32_t n = kevent(efd, NULL, 0, activeEvs, kMaxEvents, &timeout);
    printf("kqueue return %d\n", n);
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

void sapphiredb::common::Kqueue::send(){
    if(!this->empty()){
        updateEvents(this->epollfd, this->listenfd, KQUEUE_WRITE_EVENT, false);
    }
}

void sapphiredb::common::Kqueue::recv(){
    updateEvents(this->epollfd, this->listenfd, KQUEUE_READ_EVENT, false);
}

void sapphiredb::common::Kqueue::loop_once(uint32_t waitms){
    kqueue_loop_once(this->epollfd, this->listenfd, waitms);
}
