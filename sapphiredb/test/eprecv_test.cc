// kqueue test
// send port 19998
// recv port 19999
#include "common/kqueue.h"
#include "common/net.h"
#include "common/thread_pool.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include<string.h>
#include<stdio.h>

int main(){
    sapphiredb::common::Netcon* kque = new sapphiredb::common::Kqueue("127.0.0.1", 19999, sapphiredb::common::Netcon::IPV4, 10, 1024, 20);
    kque->listenp();
    sapphiredb::common::ThreadPool epoll_loop(1);
    epoll_loop.enqueue([&](){
            while(1){
                kque->loop_once(1000);
            }
        });
    while(1){
        if(kque->getData()->len == 0);
        else {
            ::std::cout << *(kque->getData()->buf) << ::std::endl;
            ::std::cout << kque->getData()->len << ::std::endl;
            ::std::cout << kque->getData()->buf->size() << ::std::endl;
        }
        sleep(1);
    }


    return 0;
}
