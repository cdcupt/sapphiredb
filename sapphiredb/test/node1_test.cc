#include "common/thread_pool.h"
#include "raft/node.h"
#include <unistd.h>
#include <string>
#include <iostream>

int main(){
    sapphiredb::raft::Timeout* timeout = new sapphiredb::raft::Timeout(500, 1500);
    sapphiredb::raft::Config* conf = new sapphiredb::raft::Config(1, "node1.log", timeout, ::std::pair<::std::string, uint32_t>("127.0.0.1", 19997), ::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>>());
    sapphiredb::raft::Node* node = new sapphiredb::raft::Node(*conf);
    sapphiredb::common::ThreadPool propose(1);
    propose.enqueue([&](){
        ::std::string str;
        while(::std::cin >> str){
            node->propose(str);
        }
    });
    node->run();
    return 0;
}
