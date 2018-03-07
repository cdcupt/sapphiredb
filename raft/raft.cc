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

void Raft::stepLeader(raftpb::Message msg){
    switch(msg.Type){
        case raftpb::MsgBeat :
            {
                this.bcastHeartbeat();
                return;
            }
        //proactively check for quorum
        case raftpb::MsgCheckQuorum :
            {
                if(!this.checkQuorumActive()){
                    this.becomeFollower(this._currentTerm, 0);
                }
                return;
            }
    }

    pr = this.getProgress(msg.From);
    if(pr == nullptr){
        //TODO
        return;
    }

    switch(msg.Type){
        case raftpb::MsgHeartbeatResp :
            {
                pr.setRecentActive();

                if(pr.getMatch() < this._lastApplied){
                    this.sendAppend(msg.From);
                }
            }
        case raftpb::MsgTransferLeader :
            {

            }
    }
}

void Raft::commitTo(uint64_t commit){
    if(this._commitIndex < commit){
        if(this._lastApplied < commit){
            //TODO error log
        }
        this._commitIndex = commit;
    }
}

int32_t grantMe(uint64_t id, raftpb::MessageType t, bool v){
    //TODO log

    if(this._votes.find(id) == this._votes.end()){
        this._votes[id] = v;
    }

    int32_t granted = 0;
    for(auto it = this._votes.begin(); it != this._votes.end(); ++it){
        if(it->second > 0) ++granted;
    }

    return granted;
}

void Raft::stepCandidate(raftpb::Message msg){
    switch(msg.Type){
        case raftpb::MsgHeartbeat:
            {
                this.becomeFollower(msg.Term, msg.From);
                this.commitTo(msg.Commit);

                raftpb::Message tmpMsg;
                tmpMsg.set_From(this._id);
                tmpMsg.set_To(msg.From);
                tmpMsg.set_Type(raftpb::MsgHeartbeatResp);
                tmpMsg.set_Context(msg.Context);

                this.send(tmpMsg);
                break;
            }
        case raftpb::MsgVoteResp:
            {
                int32_t grant = this.grantMe(msg.From, msg.Type, !msg.Reject);
                //TODO log
                switch(this.quorum()){
                    case grant:
                        {
                            this.becomeLeader();
                            this.bcastAppend();
                            break;
                        }
                    case this._votes.size() - grant:
                        {
                            this.becomeFollower(this._currentTerm, 0);
                            break;
                        }
                }
                break;
            }
    }
}

void Raft::stepFollower(raftpb::Message msg){
    switch(msg.Type){
        case raftpb::MsgHeartbeat:
            {
                this._electionElapsed = 0;
                this._leader = msg.From;
                this.commitTo(msg.Commit);

                raftpb::Message tmpMsg;
                tmpMsg.set_From(this._id);
                tmpMsg.set_To(msg.From);
                tmpMsg.set_Type(raftpb::MsgHeartbeatResp);
                tmpMsg.set_Context(msg.Context);

                this.send(tmpMsg);
                break;
            }
        case raftpb::MsgSnap:
            {

            }
        case raftpb::MsgTransferLeader:
            {
                if(this._leader == 0){
                    //TODO error log
                    return;
                }
                msg.set_To(this._leader);
                this.send(msg);
            }
    }
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

void Raft::send(raftpb::Message msg){
    if(msg.Type == raftpb::MsgVote || msg.Type == raftpb::MsgVoteResp){
        if(msg.Term == 0){
            //TODO error log
        }
    }
    else{
        if(msg.Term != 0){
            //TODO error log
        }

        //TODO MsgProp MsgReadIndex
    }

    this._msgs = this._msgs + msg;
}

void Raft::sendHeartbeat(uint64_t to, std::string& ctx){
    uint64_t commit = min(this.getProgress(to)._match, this._commitIndex);

    raftpb::Message msg;
    msg.set_From(this._id);
    msg.set_To(to);
    msg.set_Type(raftpb::MsgHeartbeat);
    msg.set_Commit(commit);
    msg.set_Context(ctx);

    this.send(msg);
}

void Raft::forEachProgress(unordered_map<uint64_t, Progress> prs,
        std::function<void(uint64_t, Progress&)> func){
    for(auto it = prs.begin(); it!=prs.end(); ++it){
        func(it->first, it->second);
    }
}

void Raft::bcastHeartbeat(){
    this.forEachProgress(this._prs,
            [](uint64_t id, Progress _){
                if(id == this._id) return;

                this.sendHeartbeat(id, std::string(""));
            })
}

void Raft::sendAppend(uint64_t to){

}
