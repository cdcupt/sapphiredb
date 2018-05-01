#include "raft/progress.h"

sapphiredb::raft::ProgressStateType sapphiredb::raft::Progress::getState(){
    return this->_state;
}

void sapphiredb::raft::Progress::resetState(sapphiredb::raft::ProgressStateType state){
    this->_pendingSnapshot = 0;
    this->_state = state;
}

void sapphiredb::raft::Progress::becomeProbe(){
    if(this->_state == ProgressStateSnapshot){
        uint64_t tmpPendingSnapshot = this->_pendingSnapshot;
        this->resetState(ProgressStateProbe);
        this->_next = ::std::max(this->_match+1, tmpPendingSnapshot+1);
    }
    else{
        this->resetState(ProgressStateProbe);
        this->_next = this->_match+1;
    }
}

void sapphiredb::raft::Progress::becomeReplicate(){
    this->resetState(ProgressStateReplicate);
    this->_next = this->_match+1;
}

void sapphiredb::raft::Progress::becomeSnapshot(uint64_t snapshoti){
    this->resetState(ProgressStateSnapshot);
    this->_pendingSnapshot = snapshoti;
}

void sapphiredb::raft::Progress::setRecentActive(){
    this->_recentActive = true;
}

void sapphiredb::raft::Progress::resetRecentActive(){
    this->_recentActive = false;
}

uint64_t sapphiredb::raft::Progress::getMatch(){
    return this->_match;
}

void sapphiredb::raft::Progress::setMatch(uint64_t match){
    this->_match = match;
}

uint64_t sapphiredb::raft::Progress::getNext(){
    return this->_next;
}

void sapphiredb::raft::Progress::setNext(uint64_t next){
    this->_next = next;
}

void sapphiredb::raft::Progress::optimisticUpdate(uint64_t n){
    this->_next = n+1;
}

bool sapphiredb::raft::Progress::maybeUpdate(uint64_t n){
    if(this->_next < n+1){
        this->_next = n+1;
    }
    ::std::cout << "*****************this->_match: " << this->_match << " n: " << n << ::std::endl;
    if(this->_match < n){
        this->_match = n;
        return true;
    }
    return false;
}