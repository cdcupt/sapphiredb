#ifndef SAPPHIREDB_RAFT_NODE_H_
#define SAPPHIREDB_RAFT_NODE_H_

#include <vector>
#include <queue>

#include "raft/raft.h"
#include "common/thread_pool.h"

#define LONG_CXX11

namespace sapphiredb
{
namespace raft
{
typedef struct{
    uint64_t heartbeatTimeout;
    uint64_t elecctionTimeout;
}Timeout;

typedef struct{
    uint64_t id;
    ::std::string raftlog;
    Timeout* timeout;
    ::std::vector<uint64_t> peers;
}Config;

class Node{
private:
    Raft* raft;
    ::std::shared_ptr<spdlog::logger> logger;

    void init(Config& conf);
    void init(Config&& conf);
public:
    Node(Config& conf, ::std::string log = "../node_log");
    Node(Config&& conf, ::std::string log = "../node_log");
    ~Node();

    void run();
};
} //namespace raft
} //namespace sapphiredb

#endif
