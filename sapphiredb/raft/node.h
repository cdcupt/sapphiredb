#ifndef SAPPHIREDB_RAFT_NODE_H_
#define SAPPHIREDB_RAFT_NODE_H_

#include <vector>
#include <queue>
#include <utility>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include "raft/raft.h"
#include "common/thread_pool.h"
#include "common/uniqueid.h"
#include "common/net.h"
#include "common/kqueue.h"

#define LONG_CXX11

namespace sapphiredb
{
namespace raft
{
class Timeout{
public:
    uint64_t heartbeatTimeout;
    uint64_t electionTimeout;
public:
    Timeout(uint64_t heartbeatTimeout, uint64_t electionTimeout);
};

class Config{
public:
    uint64_t id;
    ::std::string raftlog;
    Timeout* timeout;
    ::std::pair<::std::string, uint32_t> socket;
    ::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>> peers;
public:
    Config(uint64_t id, ::std::string raftlog, Timeout* timeout, ::std::pair<::std::string, uint32_t> socket, ::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>> peers);
    ~Config();
};

class Node{
private:
    Raft* raft;
    common::Uniqueid* uid;
    ::std::shared_ptr<spdlog::logger> logger;
    sapphiredb::common::Kqueue* kque;

    //send thread mutex and condition
    ::std::mutex tsend_mutex;
    ::std::condition_variable tsend_condition;

    //receive thread mutex and condition
    ::std::mutex trecv_mutex;
    ::std::condition_variable trecv_condition;

    //receive thread mutex and condition
    ::std::mutex tbind_mutex;
    ::std::condition_variable tbind_condition;

    //receive thread mutex and condition
    ::std::mutex tstep_mutex;
    ::std::condition_variable tstep_condition;

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
