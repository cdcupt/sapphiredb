#include "raft/raft.h"

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
        msg.set_from(this->_id);
        msg.set_type(raftpb::MsgHup);
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
            msg.set_from(this->_id);
            msg.set_type(raftpb::MsgCheckQuorum);
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
        msg.set_from(this->_id);
        msg.set_type(raftpb::MsgBeat);
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
    switch(msg.type()){
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

    pr = this->getProgress(msg.from());
    if(pr == nullptr){
        //TODO
        return;
    }

    switch(msg.type()){
        case raftpb::MsgAppResp :
            {
                pr.setRecentActive();

                if(msg.reject()){
                    LOGV(WARN_LEVEL, this->logger, "%d received msgApp rejection(lastindex: %d) from %d for index %d",
                            this->_id, msg.rejecthint(), msg.from(), msg.index());
                }
            }
        case raftpb::MsgHeartbeatResp :
            {
                pr.setRecentActive();

                if(pr.getMatch() < this->_lastApplied){
                    this->sendAppend(msg.from());
                }
                break;
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
#ifdef LONG_CXX11
    for(auto it = this->_votes.begin(); it != this->_votes.end(); ++it){
        if(it->second > 0) ++granted;
    }
#else
    for(::std::unordered_map<uint64_t, int32_t>::iterator it = this->_votes.begin(); it != this->_votes.end(); ++it){
        if(it->second > 0) ++granted;
    }
#endif
    return granted;
}

void Raft::stepCandidate(raftpb::Message msg){
    switch(msg.type()){
        case raftpb::MsgHeartbeat:
            {
                this->becomeFollower(msg.term(), msg.from());
                this->commitTo(msg.commit());

                raftpb::Message tmpMsg;
                tmpMsg.set_from(this->_id);
                tmpMsg.set_to(msg.from());
                tmpMsg.set_type(raftpb::MsgHeartbeatResp);
                tmpMsg.set_context(msg.context());

                this->send(tmpMsg);
                break;
            }
        case raftpb::MsgApp:
            {
                this.becomeFollower(this._currentTerm, msg.from());

                //handle appendEntries
                if(msg.index() < this->_commitIndex){
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(this->_commitIndex);

                    this->send(tmsg);

                    return;
                }

                vector<Entries> ents;
                for(int i=0; i<msg.entries_size(); ++i){
                    Entrie ent;
                    ent.setIndex(msg.entries(i).index());
                    ent.setTerm(msg.entries(i).term());
                    ent.setOpt(msg.entries(i).data());
                    ents.push_back(ent);
                }
                if(this->tryAppend(msg.nidex(), msg.logterm(), msg.commit(), ents) > 0){
                    raftpb::Message tmsg;
                    tmsp.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(this->_commitIndex);
                    this.send(tmsg);
                }
                else{
                    LOGV(INFO_LEVEL, this->logger, "%d rejected msgApp [logterm: %d, index: %d] from %d",
                            this->_id, msg.logterm(), msg.index(), msg.from());

                    raftpb::Message tmsg;
                    tmsp.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_reject(true);
                    tmsg.set_index(this->_commitIndex);
                    this.send(tmsg);
                }
                return;
            }
        case raftpb::MsgVoteResp:
            {
                int32_t grant = this->grantMe(msg.from(), msg.type(), !msg.reject());
                LOGV(INFO_LEVEL, this->logger, "%d [quorum: %d] has received %d %s votes and %d vote rejections",
                        this->_id, this->quorum(), grant, msg.type(), len(this->_votes)-grant);
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
    switch(msg.type()){
        case raftpb::MsgHeartbeat:
            {
                this->_electionElapsed = 0;
                this->_leader = msg.from();
                this->commitTo(msg.commit());

                raftpb::Message tmsg;
                tmsg.set_from(this->_id);
                tmsg.set_to(msg.from());
                tmsg.set_type(raftpb::MsgHeartbeatResp);
                tmsg.set_context(msg.context());

                this->send(tmsg);
                break;
            }
        case raftpb::MsgApp:
            {
                this->_electionElapsed = 0;
                this->leader = msg.from();

                //handle appendEntries
                if(msg.index() < this->_commitIndex){
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(this->_commitIndex);

                    this->send(tmsg);

                    return;
                }

                vector<Entries> ents;
                for(int i=0; i<msg.entries_size(); ++i){
                    Entrie ent;
                    ent.setIndex(msg.entries(i).index());
                    ent.setTerm(msg.entries(i).term());
                    ent.setOpt(msg.entries(i).data());
                    ents.push_back(ent);
                }
                if(this->tryAppend(msg.nidex(), msg.logterm(), msg.commit(), ents) > 0){
                    raftpb::Message tmsg;
                    tmsp.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(this->_commitIndex);
                    this.send(tmsg);
                }
                else{
                    LOGV(INFO_LEVEL, this->logger, "%d rejected msgApp [logterm: %d, index: %d] from %d",
                            this->_id, msg.logterm(), msg.index(), msg.from());

                    raftpb::Message tmsg;
                    tmsp.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_reject(true);
                    tmsg.set_index(this->_commitIndex);
                    this.send(tmsg);
                }

                return;
            }
        case raftpb::MsgSnap:
            {
                //TODO
                break;
            }
        case raftpb::MsgTransferLeader:
            {
                if(this->_leader == 0){
                    LOGV(ERROR_LEVEL, this->logger, "id: %d can't transferleader because leader is 0 in term %d", this->_id, this->currentTerm);
                    return;
                }
                msg.set_to(this->_leader);
                this->send(msg);
            }
    }
}

//send RPC whit entries(or nothing) to the given peer.
pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
        uint64_t preLogTerm, vector<Entrie> entries, uint64_t leaderCommit){

    if(term < this->_currentTerm) return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size()<=preLogIndex || this->_entries[preLogIndex].getTerm() != preLogTerm){
        return pair<uint64_t, bool>(this->_currentTerm, false);
    }

    //Heartbeat RPC
    if(entries.empty()) return pair<uint64_t, bool>(this->_currentTerm, true);

    for(int i=preLogIndex; i<this->_entries.size(); ++i){
        if(this->_entries[i].getTerm() != entries[i-preLogIndex].getTerm()){
            this->_entries = vector<Entrie>(this->_entries.begin(), this->_entries.begin()+i);
            this->_entries += vector<Entrie>(entries.begin()+i-preLogIndex, entries.end());
            break;
        }
    }

    if(leaderCommit > this->_commitIndex) {
        this->_commitIndex = std::min(leaderCommit, entries[entries.size()-1].getIndex());
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

    if(this->_entries.size() < lastLogIndex &&
            this->_entries[this->_entries.size()-1].getTerm() > lastLogTerm)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size() == lastLogIndex &&
            this->_entries[this->_entries.size()-1].getTerm() != lastLogTerm)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size() > lastLogIndex)
        return pair<uint64_t, bool>(this->_currentTerm, false);

    //vote the candidate
    this->_vote = candidateId;
    return pair<uint64_t, bool>(this->_currentTerm, true);
}

void Raft::send(raftpb::Message msg){
    if(msg.type() == raftpb::MsgVote || msg.type() == raftpb::MsgVoteResp){
        if(msg.term() == 0){
            LOGV(ERROR_LEVEL, this->logger, "term should be set when sending %s", msg.type());
        }
    }
    else{
        if(msg.term() != 0){
            LOGV(ERROR_LEVEL, this->logger, "term should not be set when sending %s with msg term %d", msg.type(), msg.term());
        }

        //TODO MsgProp MsgReadIndex
    }

    this->_msgs = this->_msgs + msg;
}

void Raft::sendHeartbeat(uint64_t to, std::string& ctx){
    uint64_t commit = min(this->getProgress(to)._match, this->_commitIndex);

    raftpb::Message msg;
    msg.set_from(this->_id);
    msg.set_to(to);
    msg.set_type(raftpb::MsgHeartbeat);
    msg.set_commit(commit);
    msg.set_context(ctx);

    this->send(msg);
}

void Raft::forEachProgress(unordered_map<uint64_t, Progress> prs,
        std::function<void(uint64_t, Progress&)> func){
#ifdef LONG_CXX11
    for(auto it = prs.begin(); it!=prs.end(); ++it){
        func(it->first, it->second);
    }
#else
    for(::std::unordered_map<uint64_t, int32_t>::iterator it = prs.begin(); it!=prs.end(); ++it){
        func(it->first, it->second);
    }
#endif
}

void Raft::bcastHeartbeat(){
    this->forEachProgress(this->_prs,
            [](uint64_t id, Progress _){
                if(id == this->_id) return;
                this->sendHeartbeat(id, std::string(""));
            })
}

//message box approach sendAppend
void Raft::sendAppend(uint64_t to){
    raftpb::Message msg;
    msg.set_to(to);

    //TODO send snapshot if we failed to get term or entries

    msg.set_type(raftpb::MsgApp);
    msg.set_index(this->_prs[to].getNext()-1);
    msg.set_logterm = this->_entries[this->_entries.size()-1].getTerm();

    for(int i=this->_prs[to]._index; i<this->_entries.size(); ++i){
        raftpb::Message::Entry* entry = msg.add_entries();
        entry->set_type(raftpb::EntryType::EntryNormal);
        entry->set_term(this->_entries[i].getTerm());
        entry->set_index(this->_entries[i].getIndex());
        entry->set_data(this->_entries[i].getOpt());
    }

    msg.set_commit = this->_commitIndex;

    if(msg.entries_size() > 0){
        switch(this->_prs[to]._getState()){
            case ProgressStateReplicate:
                {
                    this->_prs[to].optimisticUpdate(msg.entries(msg.entries_size()-1).index());
                    break;
                }
            case ProgressStateProbe:
                {
                    //TODO
                    break;
                }
            default:
                {
                    LOGV(ERROR_LEVEL, this->logger, "%d is sending append in unhandled state %s",
                            this->_id, this->_prs[to]._state);
                }
        }
    }
    this->send(msg);
}

//appent entries to local entries
//success return index, failed return 0
#ifdef LANG_CXX11
uint64_t Raft::tryAppend(uint64_t&& index, uint64_t&& logTerm, uint64_t&& committed, vector<Entrie>&& ents){
    if(this._prs.size() >= index && this._prs[index-1].getTerm() == logTerm){
        newIndex = index + ents.size();
        this._entries += ents;

        return newIndex;
    }
    return 0;
}
#else
uint64_t Raft::tryAppend(uint64_t index, uint64_t logTerm, uint64_t committed, vector<Entrie> ents){
    if(this._prs.size() >= index && this._prs[index-1].getTerm() == logTerm){
        newIndex = index + ents.size();
        this._entries += ents;

        return newIndex;
    }
    return 0;
}
#endif
