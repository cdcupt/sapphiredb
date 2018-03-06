#ifndef SAPPHIREDB_RAFT_RAFT_H_
#define SAPPHIREDB_RAFT_RAFT_H_

#include <functional>
#include <unordered_map>
#include <vector>
#include <<utility>>
#include <mutex>

//namespace sapphiredb
//{
//namespace raft
//{

enum State{
    STATE_LEADER = 1;
    STATE_CANDIDATE = 2;
    SATTE_FOLLOWER  =3;
}

class Raft;
class Raftlog;
class Progress;

template<typename T>
class Entire{
private:
    uint64_t _index;
    uint64_t _term;
    T _opt;
public:
    uint64_t getTerm(){
        return this._term;
    }
    uint64_T getIndex(){
        return this._index;
    }
};

//template<typename T, uint32_t MAXPRS>
class Raft{
private:
    //
    std::unordered_map<uint64_t, Progress> prs;

    //persist on all server
    uint64_t _currentTerm;
    uint64_t _vote;
    uint64_t _id;
    bool isLeader;
    vector<Entire> _entires;

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
    std::function<void()> _step;
    std::function<void()> _tick;

    //random election time
    uint32_t _randomizedElectionTimeout;

    void stepDown();
    void resetRandomizedElectionTimeout();
    void reset(uint64_t term);
    void quorum();
public:
    Raft();
    ~Raft();

    void tickElection();
    void tickHeartbeat();
    void becomeCandidate();
    void becomeLeader();

    //two main RPC
    pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
            uint64_t preLogTerm, vector<Entire> entires, uint64_t leaderCommit);
    pair<uint64_t, bool> requestVote(uint64_t term, uint64_t candidateId,
            uint64_t lastLogIndex, uint64_t lastLogTerm);
};
//} //namespace raft
//} //namespace sapphiredb

#endif