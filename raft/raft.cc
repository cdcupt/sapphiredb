#include "raft.h"

void Raft::stepDown(){
    this._step = stepFollower;
    this.reset(this._term);
    this._state = STATE_FOLLOWER;
    //TODO logger
}

void Raft::resetRandomizedElectionTimeout(){
    this._randomizedElectionTimeout = r.electionTimeout + //TODO
}

void Raft::reset(uint64_t term){
    if(this._term != term){
        this._term = term;
        this._vote = 0;//TODO(Invalid representation)
    }
    this._leader = 0;//TODO(Invalid representation)

    this._electionElapsed = 0;
    this._heartbeatElapsed = 0;
    this.resetRandomizedElectionTimeout();
}

void Raft::tickElection(){
    ++this._electionElapsed;
    if(pastElectionTimeout()){
        this._electionElapsed = 0;
        //this.becomeCandidate();
        //TODO Step
    }
}

void Raft::tickHeartbeat(){
    ++this._heartbeatElapsed;
    if(this._state != STATE_LEADER){
        ret = TICK_HEARTBEAT_FAILED;
        LOG_ERROR("Other members can't generate a heartbeat apart from the leader, ret=%d", ret);
        return ret;
    }
    if(this._heartbeatElasped >= this._heartbeatTimeout){
        this._heartbeatElapsed = 0;
        //TODO Step
    }
}

void Raft::becomeCandidate(){
    if(this._state == STATE_LEADER){
        ret = TRANS_TO_CANDIDATE_FAILED;
        LOG_ERROR("leader can't be converted diretly into candidate, ret=%d", ret);
        return ret;
    }
    this._step = stepCandidate;
    this.reset(this._term + 1);
    this._tick = this.tickElection;
    this._vote = this._id;
    this._state = STATE_CANDIDATE;
    //TODO logger
}

void Raft::becomeLeader(){
    if(this._state == STATE_FOLLOWER){
        ret = TRANS_TO_LEADER_FAILED;
        LOG_ERROR("follower can't be converted diretly into leader, ret=%d", ret);
        return ret;
    }
    this._step = stepLeader;
    this.reset(this._term);
    this._leader = this._id;
    this._state = STATE_LEADER;
    //TODO(Additional log)
    //TODO logger
}

void Raft::quorum(){
    return this.prs.size()/2+1;
}

//send RPC whit entries(or nothing) to the given peer.
pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
        uint64_t preLogTerm, vector<Entire> entires, uint64_t leaderCommit){

    if(term < this._currentTerm) return pair<uint64_t, bool>(this._currentTerm, false);

    if(this._entires.size()<=preLogIndex || this._entires[preLogIndex].getTerm() != preLogTerm){
        return pair<uint64_t, bool>(this._currentTerm, false);
    }

    //Heartbeat RPC
    if(entires.empty()) return pair<uint64_t, bool>(this._currentTerm, true);

    for(int i=preLogIndex; i<this._entires.size(); ++i){
        if(this._entires[i].getTerm() != entires[i-preLogIndex].getTerm()){
            this._entires = vector<Entire>(this._entires.begin(), this._entires.begin()+i);
            this._entires += vector<Entire>(entires.begin()+i-preLogIndex, entires.end());
            break;
        }
    }

    if(leaderCommit > this._commitIndex) {
        this._commitIndex = std::min(leaderCommit, entires[entires.size()-1].getIndex());
    }

    return pair<uint64_t, bool>(this._currentTerm, true);
}

// send RPC with request other members to vote
pair<uint64_t, bool> requestVote(uint64_t term, uint64_t candidateId,
        uint64_t lastLogIndex, uint64_t lastLogTerm){

    if(term < this._currentTerm) return pair<uint64_t, bool>(this._currentTerm, false);

    if(this._state==STATE_LEADER || this._state==STATE_CANDIDATE)
        return pair<uint64_t, bool>(this._currentTerm, false);

    if(this._vote != 0) return pair<uint64_t, bool>(this._currentTerm, false);

    if(this._entires.size() < lastLogIndex &&
            this._entires[this._entires.size()-1].getTerm() > lastLogTerm)
        return pair<uint64_t, bool>(this._currentTerm, false);

    if(this._entires.size() == lastLogIndex &&
            this._entires[this._entires.size()-1].getTerm() != lastLogTerm)
        return pair<uint64_t, bool>(this._currentTerm, false);

    if(this._entires.size() > lastLogIndex)
        return pair<uint64_t, bool>(this._currentTerm, false);

    //vote the candidate
    this._vote = candidateId;
    return pair<uint64_t, bool>(this._currentTerm, true);
}

void Raft::bcastHeartbeat(){

}
