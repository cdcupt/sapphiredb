#ifndef SAPPHIREDB_RAFT_RAFT_H_
#define SAPPHIREDB_RAFT_RAFT_H_

#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <mutex>
#include <random>
#include <ctime>

#include "raft/progress.h"
#include "raft/raftpb/raftpb.pb.h"

#define LONG_CXX11

namespace sapphiredb
{
namespace raft
{

enum State{
    STATE_LEADER = 1,
    STATE_CANDIDATE = 2,
    SATTE_FOLLOWER  =3
};

uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0);

class Raft;
class Raftlog;
class Progress;

template<typename T>
class Entrie{
private:
    uint64_t _index;
    uint64_t _term;
    ::std::string _opt;
public:
    uint64_t getTerm(){
        return this._term;
    }
    void setTerm(uint64_t term){
        this._term = term;
    }

    uint64_t getIndex(){
        return this._index;
    }
    void setIndex(uint64_t index){
        this._index = index;
    }

    ::std::string getOpt(){
        return this._opt;
    }
    void setOpt(::std::string opt){
        this._opt = opt;
    }
};

//template<typename T, uint32_t MAXPRS>
class Raft{
private:
    //other members' information
    ::std::unordered_map<uint64_t, Progress> prs;

    //persist on all server
    uint64_t _currentTerm;
    uint64_t _vote;
    uint64_t _id;
    bool isLeader;
    FILE* looger;
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
    ::std::function<void(raftpb::Message msg)> _step;
    ::std::function<void()> _tick;

    //random election time
    uint32_t _randomizedElectionTimeout;

    //check quorum
    bool _checkQuorum;

    void stepDown();
    void resetRandomizedElectionTimeout();
    void reset(uint64_t term);
    void quorum();
    void sendHeartbeat(uint64_t to, std::string& ctx);
    void forEachProgress(::std::unordered_map<uint64_t, Progress> prs,
            std::function<void(uint64_t, Progress&)> func);
    void commitTo(uint64_t commit);
    void send(raftpb::Message msg);
    void stepLeader(raftpb::Message msg);
    void stepCandidate(raftpb::Message msg);
    int32_t grantMe(uint64_t id, raftpb::MessageType t, bool v);
    void stepFollower(raftpb::Message msg);

    //try to modify constant data
#ifdef LONG_CXX11
    uint64_t Raft::tryAppend(uint64_t&& index, uint64_t&& logTerm, uint64_t&& committed, vector<Entrie>&& ents);
#else
    uint64_t Raft::tryAppend(uint64_t index, uint64_t logTerm, uint64_t committed, vector<Entrie> ents);
#endif

public:
    Raft();
    ~Raft();

    void tickElection();
    void tickHeartbeat();
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
};
} //namespace raft
} //namespace sapphiredb

#endif
