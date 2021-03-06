#include "common/thread_pool.h"
#include "raft/node.h"
#include <unistd.h>
#include <string>
#include <iostream>

int main(){
    sapphiredb::raft::Timeout* timeout = new sapphiredb::raft::Timeout(500, 1500);
    ::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>> peers;
    peers.push_back(::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>(1, ::std::pair<::std::string, uint32_t>("127.0.0.1", 19997)));
    //peers.push_back(::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>(2, ::std::pair<::std::string, uint32_t>("127.0.0.1", 19998)));
    sapphiredb::raft::Config* conf = new sapphiredb::raft::Config(3, "node3.log", timeout, ::std::pair<::std::string, uint32_t>("127.0.0.1", 19999), peers);
    sapphiredb::raft::Node* node = new sapphiredb::raft::Node(*conf);
    node->run();
    return 0;
}
