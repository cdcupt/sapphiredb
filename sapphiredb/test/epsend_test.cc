// kqueue test
// send port 19998
// recv port 19999
#include "common/kqueue.h"
#include "common/net.h"
#include "common/thread_pool.h"
#include <unistd.h>
#include <string>
#include <iostream>

int main(){
    sapphiredb::common::Netcon* kque = new sapphiredb::common::Kqueue("127.0.0.1", 19998, sapphiredb::common::Netcon::IPV4, 10, 1024, 20);
    kque->conn("127.0.0.1", 19999, 0);
    sapphiredb::common::ThreadPool epoll_loop(1);
    epoll_loop.enqueue([&](){
            while(1){
                kque->loop_once(1000);
            }
        });
    while(1){
        ::std::cout << "chat to 19999 : " << ::std::endl;
        ::std::string chat;
        ::std::cin >> chat;
        if(!(kque->stackingData(chat))){
            sleep(1);
            continue;
        }
        ::std::cout << "****send******" << ::std::endl;
        kque->send(0);
    }


    return 0;
}
