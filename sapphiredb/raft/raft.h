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

#include "raft/progress.h"
#include "raft/raftpb/raftpb.pb.h"
#include "spdlog/include/spdlog/spdlog.h"

#define LONG_CXX11

namespace sapphiredb
{
namespace raft
{

enum State{
    STATE_LEADER = 1,
    STATE_CANDIDATE = 2,
    STATE_FOLLOWER  =3
};

uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0);

class Raft;
class Raftlog;
class Progress;

class Entrie{
private:
    uint64_t _index;
    uint64_t _term;
    ::std::string _opt;
public:
    uint64_t getTerm(){
        return this->_term;
    }
    void setTerm(uint64_t term){
        this->_term = term;
    }

    uint64_t getIndex(){
        return this->_index;
    }
    void setIndex(uint64_t index){
        this->_index = index;
    }

    ::std::string getOpt(){
        return this->_opt;
    }
    void setOpt(::std::string opt){
        this->_opt = opt;
    }
};

void stepLeader(sapphiredb::raft::Raft* r, raftpb::Message msg);
void stepCandidate(sapphiredb::raft::Raft* r, raftpb::Message msg);
void stepFollower(sapphiredb::raft::Raft* r, raftpb::Message msg);

void tickElection(sapphiredb::raft::Raft* r);
void tickHeartbeat(sapphiredb::raft::Raft* r);

class Raft{
private:
    //other members' information
    ::std::unordered_map<uint64_t, Progress> prs;

    //persist on all server
    uint64_t _currentTerm;
    uint64_t _vote;
    uint64_t _id;
    bool isLeader;
    std::shared_ptr<spdlog::logger> logger;
    ::std::vector<Entrie> _entries;
    //prs represents all follower's progress in the view of the leader.
    ::std::unordered_map<uint64_t, Progress> _prs;
    ::std::unordered_map<uint64_t, int32_t> _votes;

    //constant change on all server
    uint64_t _commitIndex;
    uint64_t _lastApplied;

    //other memeber
    uint64_t _leader;
    uint32_t _state;

    //heartbeat timer
    uint32_t _heartbeatElapsed;
    uint32_t _heartbeatTimeout;

    //election timer
    uint32_t _electionElapsed;
    uint32_t _electionTimeout;

    //func interface
    ::std::function<void(sapphiredb::raft::Raft*, raftpb::Message msg)> _step;
    ::std::function<void(sapphiredb::raft::Raft* r)> _tick;

    //random election time
    uint32_t _randomizedElectionTimeout;

    //check quorum
    bool _checkQuorum;

    //message box
    ::std::vector<raftpb::Message> _msgs;

    void resetRandomizedElectionTimeout();
    void reset(uint64_t term);
    uint32_t quorum();
    void sendHeartbeat(uint64_t to, std::string ctx);
    void forEachProgress(::std::unordered_map<uint64_t, Progress> prs,
            std::function<void(sapphiredb::raft::Raft*, uint64_t, Progress&)> func);
    void commitTo(uint64_t commit);
    void send(raftpb::Message msg);
    int32_t grantMe(uint64_t id, raftpb::MessageType t, bool v);
    bool pastElectionTimeout();
    bool checkQuorumActive();
    void generalStep(raftpb::Message msg);

    //try to modify constant data
    uint64_t tryAppend(const uint64_t& index, const uint64_t& logTerm, const uint64_t& committed, const ::std::vector<Entrie>& ents);

public:
    Raft(uint64_t id, ::std::string path = "../raft_log", uint32_t heartbeatTimeout = 10, uint32_t electionTimeout = 150);
    ~Raft();

    friend void stepLeader(sapphiredb::raft::Raft* r, raftpb::Message msg);
    friend void stepCandidate(sapphiredb::raft::Raft* r, raftpb::Message msg);
    friend void stepFollower(sapphiredb::raft::Raft* r, raftpb::Message msg);

    friend void tickElection(sapphiredb::raft::Raft* r);
    friend void tickHeartbeat(sapphiredb::raft::Raft* r);

    void stepDown(uint64_t term, uint64_t leader);
    void becomeCandidate();
    void becomeLeader();

    //two main RPC
    ::std::pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
            uint64_t preLogTerm, ::std::vector<Entrie> entries, uint64_t leaderCommit);
    ::std::pair<uint64_t, bool> requestVote(uint64_t term, uint64_t candidateId,
            uint64_t lastLogIndex, uint64_t lastLogTerm);

    //message box approach
    void sendAppend(uint64_t to);

    void bcastHeartbeat();
    void bcastAppend();
};
} //namespace raft
} //namespace sapphiredb

#endif
