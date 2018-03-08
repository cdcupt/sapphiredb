#include "raft.h"

uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0)
{
    static std::default_random_engine e(seed);
    static std::uniform_real_distribution<uint32_t> u(min, max);
    return u(e);
}

void Raft::stepDown(){
    this->_step = stepFollower;
    this->reset(this._term);
    this->_state = STATE_FOLLOWER;

    switch(this->_state){
        case STATE_LEADER : LOGV(WARN_LEVEL, this->logger, "id: %d step dowm from leader to follower in term %d", this->_id, this->_currentTerm); break;
        case STATE_CANDIDATE : LOGV(WARN_LEVEL, this->logger, "id: %d step dowm from candidate to follower in term %d", this->_id, this->_currentTerm); break;
    }

}

void Raft::resetRandomizedElectionTimeout(){
    this->_randomizedElectionTimeout = this->_electionTimeout + rand(0, this->_elecTimeout, time(NULL));
}

void Raft::reset(uint64_t term){
    if(this->_term != term){
        this->_term = term;
        this->_vote = 0;//TODO(Invalid representation)
    }
    this->_leader = 0;//TODO(Invalid representation)

    this->_electionElapsed = 0;
    this->_heartbeatElapsed = 0;
    this->resetRandomizedElectionTimeout();
}

void Raft::tickElection(){
    ++this->_electionElapsed;
    if(pastElectionTimeout()){
        this->_electionElapsed = 0;
        raftpb::Message msg;
        msg.set_From(this->_id);
        msg.set_Type(raftpb::MsgHup);
        this->generalStep(msg);
    }
}

void Raft::tickHeartbeat(){
    ++this->_heartbeatElapsed;
    ++this->_electionElapsed;

    if(this->_electionElapsed >= this->_electionTimeout){
        this._electionElapsed = 0;

        if(this->_checkQuorum){
            raftpb::Message msg;
            msg.set_From(this->_id);
            msg.set_Type(raftpb::MsgCheckQuorum);
            this->generalStep(msg);
        }

        //TODO leader transfer
    }
    if(this->_state != STATE_LEADER){
        LOGV(ERROR_LEVEL, this->logger, "id: %d produce invaild heartbeat in term %d.", this->_id, this->_currentTerm);
        return;
    }
    if(this->_heartbeatElasped >= this->_heartbeatTimeout){
        this->_heartbeatElapsed = 0;
        raftpb::Message msg;
        msg.set_From(this->_id);
        msg.set_Type(raftpb::MsgBeat);
        this->generalStep(msg);
    }
}

void Raft::becomeCandidate(){
    if(this->_state == STATE_LEADER){
        LOGV(ERROR_LEVEL, this->logger, "id: %d try to become candidate in term %d.", this->_id, this->_currentTerm);
        return;
    }
    this->_step = stepCandidate;
    this->reset(this->_term + 1);
    this->_tick = this->tickElection;
    this->_vote = this->_id;
    this->_state = STATE_CANDIDATE;
    LOGV(WARN_LEVEL, this->logger, "id: %d from follower change to candidate in term %d.", this->_id, this->_currentTerm);
}

void Raft::becomeLeader(){
    if(this->_state == STATE_FOLLOWER){
        LOGV(ERROR_LEVEL, this->logger, "id: %d try to change to candidate from leader in term %d.", this->_id, this->_currentTerm);
        return;
    }
    this->_step = stepLeader;
    this->reset(this->_term);
    this->_leader = this->_id;
    this->_state = STATE_LEADER;
    //TODO(Additional log)
    LOGV(WARN_LEVEL, this->logger, "id : %d from candidate change to leader in term %d.", this->_id, this->_currentTerm);
}

void Raft::quorum(){
    return this->prs.size()/2+1;
}

void Raft::stepLeader(raftpb::Message msg){
    switch(msg.Type){
        case raftpb::MsgBeat :
            {
                this->bcastHeartbeat();
                return;
            }
        //proactively check for quorum
        case raftpb::MsgCheckQuorum :
            {
                if(!this->checkQuorumActive()){
                    this->becomeFollower(this->_currentTerm, 0);
                }
                return;
            }
    }

    pr = this->getProgress(msg.From);
    if(pr == nullptr){
        //TODO
        return;
    }

    switch(msg.Type){
        case raftpb::MsgHeartbeatResp :
            {
                pr.setRecentActive();

                if(pr.getMatch() < this->_lastApplied){
                    this->sendAppend(msg.From);
                }
            }
        case raftpb::MsgTransferLeader :
            {
                //TODO
            }
    }
}

void Raft::commitTo(uint64_t commit){
    if(this->_commitIndex < commit){
        if(this->_lastApplied < commit){
            LOGV(ERROR_LEVEL, this->logger, "commit(%d) is out of range [lastIndex(%d)]. Was the raft log corrupted,
                    truncated, or lost?", commit, this->_lastApplied);
        }
        this->_commitIndex = commit;
    }
}

int32_t grantMe(uint64_t id, raftpb::MessageType t, bool v){
    if(v){
        LOGV(INFO_LEVEL, this->logger, "%d received %s from %d at term %d", this->_id, t, id, this->_currentTerm);
    }
    else{
        LOGV(INFO_LEVEL, this->logger, "%d received %s rejection from %d at term %d", this->_id, t, id, this->_currentTerm);
    }

    if(this->_votes.find(id) == this->_votes.end()){
        this->_votes[id] = v;
    }

    int32_t granted = 0;
    for(auto it = this->_votes.begin(); it != this->_votes.end(); ++it){
        if(it->second > 0) ++granted;
    }

    return granted;
}

void Raft::stepCandidate(raftpb::Message msg){
    switch(msg.Type){
        case raftpb::MsgHeartbeat:
            {
                this->becomeFollower(msg.Term, msg.From);
                this->commitTo(msg.Commit);

                raftpb::Message tmpMsg;
                tmpMsg.set_From(this->_id);
                tmpMsg.set_To(msg.From);
                tmpMsg.set_Type(raftpb::MsgHeartbeatResp);
                tmpMsg.set_Context(msg.Context);

                this->send(tmpMsg);
                break;
            }
        case raftpb::MsgVoteResp:
            {
                int32_t grant = this->grantMe(msg.From, msg.Type, !msg.Reject);
                LOGV(INFO_LEVEL, this->logger, "%d [quorum: %d] has received %d %s votes and %d vote rejections",
                        this->_id, this->quorum(), grant, msg.Type, len(this->_votes)-grant);
                switch(this->quorum()){
                    case grant:
                        {
                            this->becomeLeader();
                            this->bcastAppend();
                            break;
                        }
                    case this->_votes.size() - grant:
                        {
                            this->becomeFollower(this->_currentTerm, 0);
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
                this->_electionElapsed = 0;
                this->_leader = msg.From;
                this->commitTo(msg.Commit);

                raftpb::Message tmpMsg;
                tmpMsg.set_From(this->_id);
                tmpMsg.set_To(msg.From);
                tmpMsg.set_Type(raftpb::MsgHeartbeatResp);
                tmpMsg.set_Context(msg.Context);

                this->send(tmpMsg);
                break;
            }
        case raftpb::MsgSnap:
            {
                //TODO
            }
        case raftpb::MsgTransferLeader:
            {
                if(this->_leader == 0){
                    LOGV(ERROR_LEVEL, this->logger, "id: %d can't transferleader because leader is 0 in term %d", this->_id, this->currentTerm);
                    return;
                }
                msg.set_To(this->_leader);
                this->send(msg);
            }
    }
}

//send RPC whit entries(or nothing) to the given peer.
pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
        uint64_t preLogTerm, vector<Entire> entires, uint64_t leaderCommit){

    if(term < this->_currentTerm) return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entires.size()<=preLogIndex || this->_entires[preLogIndex].getTerm() != preLogTerm){
        return pair<uint64_t, bool>(this->_currentTerm, false);
    }

    //Heartbeat RPC
    if(entires.empty()) return pair<uint64_t, bool>(this->_currentTerm, true);

    for(int i=preLogIndex; i<this->_entires.size(); ++i){
        if(this->_entires[i].getTerm() != entires[i-preLogIndex].getTerm()){
            this->_entires = vector<Entire>(this->_entires.begin(), this->_entires.begin()+i);
            this->_entires += vector<Entire>(entires.begin()+i-preLogIndex, entires.end());
            break;
        }
    }

    if(leaderCommit > this->_commitIndex) {
        this->_commitIndex = std::min(leaderCommit, entires[entires.size()-1].getIndex());
    }

    return pair<uint64_t, bool>(this->_currentTerm, true);
}

// send RPC with request other members to vote
pair<uint64_t, bool> requestVote(uint64_t term, uint64_t candidateId,
        uint64_t lastLogIndex, uint64_t lastLogTerm){

    if(term < this->_currentTerm) return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_state==STATE_LEADER || this->_state==STATE_CANDIDATE)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_vote != 0) return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entires.size() < lastLogIndex &&
            this->_entires[this->_entires.size()-1].getTerm() > lastLogTerm)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entires.size() == lastLogIndex &&
            this->_entires[this->_entires.size()-1].getTerm() != lastLogTerm)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entires.size() > lastLogIndex)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    //vote the candidate
    this->_vote = candidateId;
    return pair<uint64_t, bool>(this->_currentTerm, true);
}

void Raft::send(raftpb::Message msg){
    if(msg.Type == raftpb::MsgVote || msg.Type == raftpb::MsgVoteResp){
        if(msg.Term == 0){
            LOGV(ERROR_LEVEL, this->logger, "term should be set when sending %s", msg.Type);
        }
    }
    else{
        if(msg.Term != 0){
            LOGV(ERROR_LEVEL, this->logger, "term should not be set when sending %s with msg term %d", msg.Type, msg.Term);
        }

        //TODO MsgProp MsgReadIndex
    }

    this->_msgs = this->_msgs + msg;
}

void Raft::sendHeartbeat(uint64_t to, std::string& ctx){
    uint64_t commit = min(this->getProgress(to)._match, this->_commitIndex);

    raftpb::Message msg;
    msg.set_From(this->_id);
    msg.set_To(to);
    msg.set_Type(raftpb::MsgHeartbeat);
    msg.set_Commit(commit);
    msg.set_Context(ctx);

    this->send(msg);
}

void Raft::forEachProgress(unordered_map<uint64_t, Progress> prs,
        std::function<void(uint64_t, Progress&)> func){
    for(auto it = prs.begin(); it!=prs.end(); ++it){
        func(it->first, it->second);
    }
}

void Raft::bcastHeartbeat(){
    this->forEachProgress(this->_prs,
            [](uint64_t id, Progress _){
                if(id == this->_id) return;
                this->sendHeartbeat(id, std::string(""));
            })
}

void Raft::sendAppend(uint64_t to){

}
