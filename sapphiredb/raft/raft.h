#ifndef SAPPHIREDB_RAFT_RAFT_H_
#define SAPPHIREDB_RAFT_RAFT_H_

#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <mutex>
#include <random>
#include <ctime>
#include <iostream>
#include <string>
#include <cstdio>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include "raft/progress.h"
#include "raft/raftpb/raftpb.pb.h"
#include "common/spdlog/include/spdlog/spdlog.h"
#include "raft/storage.h"

#define LONG_CXX11

namespace sapphiredb
{
namespace raft
{

enum State{
    STATE_LEADER = 1,
    STATE_CANDIDATE = 2,
    STATE_FOLLOWER  =3,
    STATE_LOCKING = 4
};

uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0);

class Raft;
class Raftlog;
class Progress;

class Sendstruct{
public:
    ::std::string msg;
    uint64_t to;

    inline Sendstruct(::std::string data, uint64_t id){
        this->msg = data;
        this->to = id;
    }
};

class Entry{
private:
    uint64_t _index;
    uint64_t _term;
    ::std::string _opt;
public:
    inline uint64_t getTerm(){
        return this->_term;
    }
    inline void setTerm(uint64_t term){
        this->_term = term;
    }

    inline uint64_t getIndex(){
        return this->_index;
    }
    inline void setIndex(uint64_t index){
        this->_index = index;
    }

    inline ::std::string getOpt(){
        return this->_opt;
    }
    inline void setOpt(::std::string opt){
        this->_opt = opt;
    }
};

void stepLeader(sapphiredb::raft::Raft* r, raftpb::Message msg);
void stepCandidate(sapphiredb::raft::Raft* r, raftpb::Message msg);
void stepFollower(sapphiredb::raft::Raft* r, raftpb::Message msg);
void stepLocking(sapphiredb::raft::Raft* r, raftpb::Message msg);

void tickElection(sapphiredb::raft::Raft* r);
void tickHeartbeat(sapphiredb::raft::Raft* r);

class Raft{
public:
    //other members' information
    ::std::unordered_map<uint64_t, Progress> prs;

    //persist on all server
    uint64_t _currentTerm;
    uint64_t _vote;
    uint64_t _id;
    bool isLeader;
    std::shared_ptr<spdlog::logger> logger;
    //prs represents all follower's progress in the view of the leader.
    ::std::unordered_map<uint64_t, Progress> _prs;
    ::std::unordered_map<uint64_t, int32_t> _votes;

    //about raftlog
    //constant change on all server
    uint64_t _commitIndex;
    uint64_t _lastApplied;
    //try to modify constant data
    uint64_t maybeAppend(const uint64_t& index, const uint64_t& logTerm, const uint64_t& committed, const ::std::vector<raftpb::Entry>& ents);
    uint64_t append(::std::vector<raftpb::Entry> ents);
    bool maybeCommit();
    void restore(raftpb::Snapshot snap);
    raftpb::Snapshot snapshot();
    uint64_t firstIndex();
    uint64_t lastIndex();
    void commitTo(uint64_t commit);
    void appliedTo(uint64_t index);
    uint64_t lastTerm();
    uint64_t term(uint64_t index);

    //unstable storage
    ::std::vector<raftpb::Entry> _entries;
    raftpb::Snapshot* _snap;
    uint64_t _offset;
    uint64_t maybeFirstIndex();
    uint64_t maybeLastIndex();
    uint64_t maybeTerm(uint64_t index);
    void stableTo(uint64_t index, uint64_t term);
    void stableSnapTo(uint64_t index);
    void truncateAndAppend(::std::vector<raftpb::Entry>& ents);

    //storage
    Storage* _storage;

    //other memeber
    uint64_t _leader;
    uint32_t _state;

    //heartbeat timer
    uint32_t _heartbeatElapsed;
    uint32_t _heartbeatTimeout;

    //election timer
    uint32_t _electionElapsed;
    uint32_t _electionTimeout;

    //locking timer
    //uint32_t _lockingElapsed;
    //uint32_t _lockingTimeout;

    //func interface
    ::std::function<void(sapphiredb::raft::Raft* r, raftpb::Message msg)> _step;
    ::std::function<void(sapphiredb::raft::Raft* r)> _tick;

    //random election time
    uint32_t _randomizedElectionTimeout;

    //check quorum
    bool _checkQuorum;

    //message box
    ::std::queue<raftpb::Message> _sendmsgs;
    ::std::queue<raftpb::Message> _recvmsgs;
    ::std::queue<uint64_t> unknownid;

    ::std::mutex sendbuf_mutex;
    ::std::mutex recvbuf_mutex;
    ::std::mutex unknownid_mutex;

    ::std::condition_variable* node_send_condition;
    ::std::condition_variable* node_recv_condition;
    ::std::condition_variable* node_bind_condition;
    ::std::condition_variable* node_step_condition;
    ::std::condition_variable* node_persist_condition;

    uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0);
    void resetRandomizedElectionTimeout();
    void reset(uint64_t term);
    uint32_t quorum();
    void sendHeartbeat(uint64_t to, std::string ctx);
    void forEachProgress(::std::unordered_map<uint64_t, Progress> prs,
            std::function<void(sapphiredb::raft::Raft*, uint64_t, Progress&)> func);
    void send(raftpb::Message msg);
    int32_t grantMe(uint64_t id, raftpb::MessageType t, bool v);
    bool pastElectionTimeout();
    bool checkQuorumActive();
    void generalStep(raftpb::Message msg);
    void appendEntry(::std::vector<raftpb::Entry> ents);
    bool propose(::std::string op);

    ::std::string serializeData(raftpb::Message msg);
    raftpb::Message deserializeData(::std::string data);

public:
    Raft(uint64_t id, ::std::condition_variable* tsend_condition = nullptr, ::std::condition_variable* trecv_condition = nullptr,
        ::std::condition_variable* tbind_condition = nullptr, ::std::condition_variable* tstep_condition = nullptr, ::std::condition_variable* tpersist_condition = nullptr,
        ::std::string path = "./raft_log", uint32_t heartbeatTimeout = 10, uint32_t electionTimeout = 150, ::std::string storage_path = "./storage.st");
    ~Raft();

    friend void stepLeader(sapphiredb::raft::Raft* r, raftpb::Message msg);
    friend void stepCandidate(sapphiredb::raft::Raft* r, raftpb::Message msg);
    friend void stepFollower(sapphiredb::raft::Raft* r, raftpb::Message msg);
    friend void stepLocking(sapphiredb::raft::Raft* r, raftpb::Message msg);

    friend void tickElection(sapphiredb::raft::Raft* r);
    friend void tickHeartbeat(sapphiredb::raft::Raft* r);

    void stepDown(uint64_t term, uint64_t leader);
    void becomeCandidate();
    void becomeLeader();
    void becomeLocking();

    //two main RPC
    ::std::pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
            uint64_t preLogTerm, ::std::vector<Entry> entries, uint64_t leaderCommit);
    ::std::pair<uint64_t, bool> requestVote(uint64_t term, uint64_t candidateId,
            uint64_t lastLogIndex, uint64_t lastLogTerm);

    //message box approach
    void sendAppend(uint64_t to);

    void bcastHeartbeat();
    void bcastHeartbeat_fast();
    void bcastAppend();

    //general API
    void tickNode(sapphiredb::raft::Raft* r);
    void stepNode();

    void sendAddNode(uint64_t to);

    void stop();

    sapphiredb::raft::Sendstruct tryPopSendbuf();
    bool tryPushRecvbuf(::std::string data);

    void addNode(uint64_t id, bool isLeader = false);
    void deleteNode(uint64_t id);

    void pushUnknownid(uint64_t& id);
    void pushUnknownid(int32_t&& id);
    int32_t popUnknownid();
    bool emptyUnknownid();

    void storageLog();

    bool Ready;

    inline ::std::string name(raftpb::MessageType e){
        switch(e){
            case raftpb::MsgTrival: return "MsgTrival";
            case raftpb::MsgHeartbeat: return "MsgHeartbeat";
            case raftpb::MsgHeartbeatResp: return "MsgHeartbeatResp";
            case raftpb::MsgVote: return "MsgVote";
            case raftpb::MsgVoteResp: return "MsgVoteResp";
            case raftpb::MsgApp: return "MsgApp";
            case raftpb::MsgAppResp: return "MsgAppResp";
            case raftpb::MsgHup: return "MsgHup";
            case raftpb::MsgCheckQuorum: return "MsgCheckQuorum";
            case raftpb::MsgTransferLeader: return "MsgTransferLeader";
            case raftpb::MsgSnap: return "MsgSnap";
            case raftpb::MsgNode: return "MsgNode";
            case raftpb::MsgNodeResp: return "MsgNodeResp";
            default: return "unknownType";
        }
    }

    inline bool isSendEmpty(){
        return this->_sendmsgs.empty();
    }

    inline bool isRecvEmpty(){
        return this->_recvmsgs.empty();
    }
};
} //namespace raft
} //namespace sapphiredb

#endif
