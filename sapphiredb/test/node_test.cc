#include <iostream>
#include <vector>
#include "raft/node.h"
#include "common/kqueue.h"
#include "common/thread_pool.h"
#include "common/uniqueid.h"

int main(){
    uint32_t n;
    std::cin >> n;
    vector<uint64_t> peers(n);
    for(int i=0; i<n; ++i){
        std::cin >> peers[i];
    }
    Uniqueid uid = Uniqueid::Instance();
    Timeout* nodetime = new Timeout(10, 150);
    Config* conf = new Config(uid, "./node_log", nodetime, peers);
    Node* node = new Node(*conf);
    node->run();

    //TODO Append Log

    //TODO ReadIndex Read and LeasedRead

    //TODO node dump
}
