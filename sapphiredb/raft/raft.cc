#include "raft/raft.h"

uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0)
{
    static std::default_random_engine e(seed);
    static std::uniform_real_distribution<double> u(min, max);
    return u(e);
}

void sapphiredb::raft::Raft::stepDown(uint64_t term, uint64_t leader){
    this->_step = sapphiredb::raft::stepFollower;
    this->reset(term);
    this->_tick = sapphiredb::raft::tickElection;
    this->_leader = leader;
    this->_state = STATE_FOLLOWER;

    switch(this->_state){
        case STATE_LEADER : logger->warn("id: {:d} step dowm from leader to follower in term {:d}", this->_id, this->_currentTerm); break;
        case STATE_CANDIDATE : logger->warn("id: {:d} step dowm from candidate to follower in term {:d}", this->_id, this->_currentTerm); break;
        default: ;
    }
}

void sapphiredb::raft::Raft::resetRandomizedElectionTimeout(){
    this->_randomizedElectionTimeout = this->_electionTimeout + rand(0, this->_electionTimeout, time(NULL));
}

void sapphiredb::raft::Raft::reset(uint64_t term){
    if(this->_currentTerm != term){
        this->_currentTerm = term;
        this->_vote = 0;//TODO(Invalid representation)
    }
    this->_leader = 0;//TODO(Invalid representation)

    this->_electionElapsed = 0;
    this->_heartbeatElapsed = 0;
    this->resetRandomizedElectionTimeout();
}

bool sapphiredb::raft::Raft::pastElectionTimeout(){
    return this->_electionElapsed >= this->_randomizedElectionTimeout;
}

void sapphiredb::raft::tickElection(sapphiredb::raft::Raft* r){
    ++r->_electionElapsed;
    if(r->pastElectionTimeout()){
        r->_electionElapsed = 0;
        raftpb::Message msg;
        msg.set_from(r->_id);
        msg.set_type(raftpb::MsgHup);
        try{
            r->generalStep(msg);
        }
        catch(...){
            r->logger->error("generalStep filed in term {:d}", r->_currentTerm);
        }
    }
}

void sapphiredb::raft::tickHeartbeat(sapphiredb::raft::Raft* r){
    ++r->_heartbeatElapsed;
    ++r->_electionElapsed;

    if(r->_electionElapsed >= r->_electionTimeout){
        r->_electionElapsed = 0;

        if(r->_checkQuorum){
            raftpb::Message msg;
            msg.set_from(r->_id);
            msg.set_type(raftpb::MsgCheckQuorum);
            try{
                r->generalStep(msg);
            }
            catch(...){
                r->logger->error("generalStep filed in term {:d}", r->_currentTerm);
            }
        }

        //TODO leader transfer
    }
    if(r->_state != STATE_LEADER){
        r->logger->error("id: {:d} produce invaild heartbeat in term {:d}.", r->_id, r->_currentTerm);
        return;
    }
    if(r->_heartbeatElapsed >= r->_heartbeatTimeout){
        r->_heartbeatElapsed = 0;
        ++r->_lockingElapsed;
        raftpb::Message msg;
        msg.set_from(r->_id);
        msg.set_type(raftpb::MsgHeartbeat);
        try{
            r->generalStep(msg);
        }
        catch(...){
            r->logger->error("generalStep filed in term {:d}", r->_currentTerm);
        }
    }
    if(r->_lockingElapsed >= r->_lockingTimeout){
        r->forEachProgress(r->_prs, [&r](sapphiredb::raft::Raft* _, uint64_t id, Progress& __){
            r->addNode(id);
        });
    }
}

void sapphiredb::raft::Raft::becomeCandidate(){
    if(this->_state == STATE_LEADER){
        logger->error("id: {:d} try to become candidate in term {:d}.", this->_id, this->_currentTerm);
        return;
    }
    this->_step = sapphiredb::raft::stepCandidate;
    this->reset(this->_currentTerm + 1);
    this->_tick = sapphiredb::raft::tickElection;
    this->_vote = this->_id;
    this->_state = STATE_CANDIDATE;
    logger->warn("id: {:d} from follower change to candidate in term {:d}.", this->_id, this->_currentTerm);
}

void sapphiredb::raft::Raft::becomeLeader(){
    if(this->_state == STATE_FOLLOWER){
        logger->error("id: {:d} try to change to candidate from leader in term {:d}.", this->_id, this->_currentTerm);
        return;
    }
    this->_step = sapphiredb::raft::stepLeader;
    this->reset(this->_currentTerm);
    this->_tick = sapphiredb::raft::tickHeartbeat;
    this->_leader = this->_id;
    this->_state = STATE_LEADER;
    //TODO(Additional log)
    logger->warn("id : {:d} from candidate change to leader in term {:d}.", this->_id, this->_currentTerm);
}

void sapphiredb::raft::Raft::becomeLocking(){
    this->_step = sapphiredb::raft::stepLocking;
    this->_tick = sapphiredb::raft::tickHeartbeat;
    this->_state = STATE_LOCKING;

    logger->warn("id : {:d} in locking.", this->_id);
}

uint32_t sapphiredb::raft::Raft::quorum(){
    return (this->prs.size())/2+1;
}

//TODO stepLocking

void sapphiredb::raft::stepLeader(sapphiredb::raft::Raft* r, raftpb::Message msg){
    switch(msg.type()){
        case raftpb::MsgHeartbeat:
            {
                r->bcastHeartbeat();
                return;
            }
        //proactively check for quorum
        case raftpb::MsgCheckQuorum:
            {
                if(!r->checkQuorumActive()){
                    r->stepDown(r->_currentTerm, 0);
                }
                return;
            }
        default: ;
    }

    if(r->_prs.find(msg.from()) == r->_prs.end()){
        //TODO
        return;
    }

    sapphiredb::raft::Progress pr = r->_prs[msg.from()];
    switch(msg.type()){
        case raftpb::MsgAppResp :
            {
                pr.setRecentActive();

                if(msg.reject()){
                    r->logger->warn("{:d} received msgApp rejection(lastindex: {:d}) from {:d} for index {:d}",
                            r->_id, msg.rejecthint(), msg.from(), msg.index());
                }
            }
        case raftpb::MsgHeartbeatResp :
            {
                pr.setRecentActive();

                if(pr.getMatch() < r->_lastApplied){
                    r->sendAppend(msg.from());
                }
                break;
            }
        case raftpb::MsgTransferLeader :
            {
                //TODO
            }
        default: ;
    }
}

void sapphiredb::raft::Raft::commitTo(uint64_t commit){
    if(this->_commitIndex < commit){
        if(this->_lastApplied < commit){
            logger->error("commit({:d}) is out of range [lastIndex({:d})]. Was the raft log corrupted, truncated, or lost?",
             commit, this->_lastApplied);
        }
        this->_commitIndex = commit;
    }
}

int32_t sapphiredb::raft::Raft::grantMe(uint64_t id, raftpb::MessageType t, bool v){
    if(v){
        logger->info("{:d} received {:s} from {:d} at term {:d}", this->_id, t, id, this->_currentTerm);
    }
    else{
        logger->info("{:d} received {:s} rejection from {:d} at term {:d}", this->_id, t, id, this->_currentTerm);
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

void sapphiredb::raft::stepCandidate(sapphiredb::raft::Raft* r, raftpb::Message msg){
    switch(msg.type()){
        case raftpb::MsgHeartbeat:
            {
                r->stepDown(msg.term(), msg.from());
                r->commitTo(msg.commit());

                raftpb::Message tmpMsg;
                tmpMsg.set_from(r->_id);
                tmpMsg.set_to(msg.from());
                tmpMsg.set_type(raftpb::MsgHeartbeatResp);
                tmpMsg.set_context(msg.context());

                r->send(tmpMsg);
                break;
            }
        case raftpb::MsgApp:
            {
                r->stepDown(r->_currentTerm, msg.from());

                //handle appendEntries
                if(msg.index() < r->_commitIndex){
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(r->_commitIndex);

                    r->send(tmsg);

                    return;
                }

                ::std::vector<Entrie> ents;
                for(int i=0; i<msg.entries_size(); ++i){
                    Entrie ent;
                    ent.setIndex(msg.entries(i).index());
                    ent.setTerm(msg.entries(i).term());
                    ent.setOpt(msg.entries(i).data());
                    ents.push_back(ent);
                }
                if(r->tryAppend(msg.index(), msg.logterm(), msg.commit(), ents) > 0){
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(r->_commitIndex);
                    r->send(tmsg);
                }
                else{
                    r->logger->info("{:d} rejected msgApp [logterm: {:d}, index: {:d}] from {:d}",
                            r->_id, msg.logterm(), msg.index(), msg.from());

                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_reject(true);
                    tmsg.set_index(r->_commitIndex);
                    r->send(tmsg);
                }
                return;
            }
        case raftpb::MsgVoteResp:
            {
                int32_t grant = r->grantMe(msg.from(), msg.type(), !msg.reject());
                r->logger->info("{:d} [quorum: {:d}] has received {:d} {:s} votes and {:d} vote rejections",
                        r->_id, r->quorum(), grant, msg.type(), r->_votes.size()-grant);
                if(r->quorum() == grant){
                    r->becomeLeader();
                    r->bcastAppend();
                }
                else if(r->quorum() == r->_votes.size() - grant){
                    r->stepDown(r->_currentTerm, 0);
                }
                break;
            }
        default: ;
    }
}

void sapphiredb::raft::stepFollower(sapphiredb::raft::Raft* r, raftpb::Message msg){
    switch(msg.type()){
        case raftpb::MsgHeartbeat:
            {
                r->_electionElapsed = 0;
                r->_leader = msg.from();
                r->commitTo(msg.commit());

                raftpb::Message tmsg;
                tmsg.set_from(r->_id);
                tmsg.set_to(msg.from());
                tmsg.set_type(raftpb::MsgHeartbeatResp);
                tmsg.set_context(msg.context());

                r->send(tmsg);
                break;
            }
        case raftpb::MsgApp:
            {
                r->_electionElapsed = 0;
                r->_leader = msg.from();

                //handle appendEntries
                if(msg.index() < r->_commitIndex){
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(r->_commitIndex);

                    r->send(tmsg);

                    return;
                }

                ::std::vector<Entrie> ents;
                for(int i=0; i<msg.entries_size(); ++i){
                    Entrie ent;
                    ent.setIndex(msg.entries(i).index());
                    ent.setTerm(msg.entries(i).term());
                    ent.setOpt(msg.entries(i).data());
                    ents.push_back(ent);
                }
                if(r->tryAppend(msg.index(), msg.logterm(), msg.commit(), ents) > 0){
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_index(r->_commitIndex);
                    r->send(tmsg);
                }
                else{
                    r->logger->info("{:d} rejected msgApp [logterm: {:d}, index: {:d}] from {:d}",
                            r->_id, msg.logterm(), msg.index(), msg.from());

                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_type(raftpb::MsgAppResp);
                    tmsg.set_reject(true);
                    tmsg.set_index(r->_commitIndex);
                    r->send(tmsg);
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
                if(r->_leader == 0){
                    r->logger->error("id: {:d} can't transferleader because leader is 0 in term {:d}", r->_id, r->_currentTerm);
                    return;
                }
                msg.set_to(r->_leader);
                r->send(msg);
            }
        default: ;
    }
}

//send RPC whit entries(or nothing) to the given peer.
/*
::std::pair<uint64_t, bool> sendAppend(uint64_t term, uint64_t id, uint64_t preLogIndex,
        uint64_t preLogTerm, ::std::vector<sapphiredb::raft::Entrie> entries, uint64_t leaderCommit){

    if(term < this->_currentTerm) return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size()<=preLogIndex || this->_entries[preLogIndex].getTerm() != preLogTerm){
        return ::std::pair<uint64_t, bool>(this->_currentTerm, false);
    }

    //Heartbeat RPC
    if(entries.empty()) return ::std::pair<uint64_t, bool>(this->_currentTerm, true);

    for(int i=preLogIndex; i<this->_entries.size(); ++i){
        if(this->_entries[i].getTerm() != entries[i-preLogIndex].getTerm()){
            this->_entries = ::std::vector<Entrie>(this->_entries.begin(), this->_entries.begin()+i);
            this->_entries += ::std::vector<Entrie>(entries.begin()+i-preLogIndex, entries.end());
            break;
        }
    }

    if(leaderCommit > this->_commitIndex) {
        this->_commitIndex = std::min(leaderCommit, entries[entries.size()-1].getIndex());
    }

    return ::std::pair<uint64_t, bool>(this->_currentTerm, true);
}
*/

// send RPC with request other members to vote
/*
::std::pair<uint64_t, bool> requestVote(uint64_t term, uint64_t candidateId,
        uint64_t lastLogIndex, uint64_t lastLogTerm){

    if(term < this->_currentTerm) return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_state==STATE_LEADER || this->_state==STATE_CANDIDATE)
        return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_vote != 0) return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size() < lastLogIndex &&
            this->_entries[this->_entries.size()-1].getTerm() > lastLogTerm)
        return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size() == lastLogIndex &&
            this->_entries[this->_entries.size()-1].getTerm() != lastLogTerm)
        return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    if(this->_entries.size() > lastLogIndex)
        return ::std::pair<uint64_t, bool>(this->_currentTerm, false);

    //vote the candidate
    this->_vote = candidateId;
    return ::std::pair<uint64_t, bool>(this->_currentTerm, true);
}
*/

void sapphiredb::raft::Raft::send(raftpb::Message msg){
    if(msg.type() == raftpb::MsgVote || msg.type() == raftpb::MsgVoteResp){
        if(msg.term() == 0){
            logger->error("term should be set when sending {:s}", msg.type());
        }
    }
    else{
        if(msg.term() != 0){
            logger->error("term should not be set when sending {:s} with msg term {:d}", msg.type(), msg.term());
        }

        //TODO MsgProp MsgReadIndex
    }

    //TODO message queue
    this->_msgs.push_back(msg);
}

void sapphiredb::raft::Raft::sendHeartbeat(uint64_t to, std::string ctx){
    uint64_t commit = ::std::min(this->_prs[to].getMatch(), this->_commitIndex);

    raftpb::Message msg;
    msg.set_from(this->_id);
    msg.set_to(to);
    msg.set_type(raftpb::MsgHeartbeat);
    msg.set_commit(commit);
    msg.set_context(ctx);

    this->send(msg);
}

void sapphiredb::raft::Raft::forEachProgress(::std::unordered_map<uint64_t, Progress> prs,
        std::function<void(sapphiredb::raft::Raft*, uint64_t, Progress&)> func){
#ifdef LONG_CXX11
    for(auto it = prs.begin(); it!=prs.end(); ++it){
        func(this, it->first, it->second);
    }
#else
    for(::std::unordered_map<uint64_t, int32_t>::iterator it = prs.begin(); it!=prs.end(); ++it){
        func(this, it->first, it->second);
    }
#endif
}

void sapphiredb::raft::Raft::bcastHeartbeat(){
#ifdef LONG_CXX11
    auto fun = [](sapphiredb::raft::Raft* r, uint64_t id, sapphiredb::raft::Progress _) { if(id == r->_id) return; r->sendHeartbeat(id, ::std::string(""));};
#else
    ::std::function<void(uint64_t id, sapphiredb::raft::Progress _)> fun = [](uint64_t id, Progress _) { if(id == this->_id) return; this->sendHeartbeat(id, std::string(""));};
#endif
    this->forEachProgress(this->_prs, fun);
}

//message box approach sendAppend
void sapphiredb::raft::Raft::sendAppend(uint64_t to){
    raftpb::Message msg;
    msg.set_to(to);

    //TODO send snapshot if we failed to get term or entries

    msg.set_type(raftpb::MsgApp);
    msg.set_index(this->_prs[to].getNext()-1);
    msg.set_logterm(this->_entries[this->_entries.size()-1].getTerm());

    for(int i=this->_prs[to].getNext()-1; i<this->_entries.size(); ++i){
        raftpb::Entry* entry = msg.add_entries();
        entry->set_type(raftpb::EntryType::EntryNormal);
        entry->set_term(this->_entries[i].getTerm());
        entry->set_index(this->_entries[i].getIndex());
        entry->set_data(this->_entries[i].getOpt());
    }

    msg.set_commit(this->_commitIndex);

    if(msg.entries_size() > 0){
        switch(this->_prs[to].getState()){
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
                    logger->error("{:d} is sending append in unhandled state {:s}",
                            this->_id, this->_prs[to].getState());
                }
        }
    }
    this->send(msg);
}

bool sapphiredb::raft::Raft::checkQuorumActive(){
    return true;
    //TODO just test
}

//appent entries to local entries
//success return index, failed return 0
uint64_t sapphiredb::raft::Raft::tryAppend(const uint64_t& index, const uint64_t& logTerm, const uint64_t& committed, const ::std::vector<Entrie>& ents){
    if(this->_prs.size() >= index && this->_commitIndex == logTerm){
        uint64_t newIndex = index + ents.size();
        this->_entries.insert(this->_entries.end(), ents.begin(), ents.end());

        return newIndex;
    }
    return 0;
}

void sapphiredb::raft::Raft::bcastAppend(){
    this->forEachProgress(this->_prs, [](sapphiredb::raft::Raft* r, uint64_t id, Progress& _){
        if(id == r->_id) return;

        r->sendAppend(id);
    });
}

void sapphiredb::raft::Raft::generalStep(raftpb::Message msg){
    if(msg.term() == 0){
        //TODO
    }
    else if(msg.term() > this->_currentTerm){
        if(msg.type() == raftpb::MsgVote && this->_checkQuorum && this->_leader != 0 &&
                this->_electionElapsed < this->_electionTimeout){
             logger->error("id: {:d} ignore vote request from {:d} [logterm: {:d}, index: {:d}]",
                     this->_id, msg.from(), msg.logterm(), msg.index());
            return;
        }

        logger->error("{:d} in term: {:d} receive a {:s} message from {:d} at term: {:d}",
            this->_id, this->_currentTerm, msg.type(), msg.from(), msg.term());
        this->stepDown(msg.term(), msg.from());
    }
    else if(msg.term() < this->_currentTerm){
        if(this->_checkQuorum && (msg.type() == raftpb::MsgHeartbeat || msg.type() == raftpb::MsgApp)){
            raftpb::Message tmsg;
            tmsg.set_to(msg.from());
            tmsg.set_type(raftpb::MsgAppResp);
            this->send(tmsg);
        }
        else{
            logger->error("{:d} in term: {:d} receive a {:s} message from {:d} at term: {:d}",
                this->_id, this->_currentTerm, msg.type(), msg.from(), msg.term());
        }
        return;
    }

    switch(msg.type()){
        case raftpb::MsgVote:
            {
                if(((0 == this->_vote) || (this->_vote = msg.from())) &&
                        (msg.logterm() > this->_commitIndex ||
                         (msg.logterm() == this->_commitIndex &&
                          msg.index() >= this->_lastApplied))){
                    logger->info("{:d} rejected msgApp [logterm: {:d}, index: {:d}] from {:d}",
                            this->_id, msg.logterm(), msg.index(), msg.from());
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_term(msg.term());
                    tmsg.set_type(raftpb::MsgVoteResp);
                    this->send(tmsg);
                    this->_electionElapsed = 0;
                    this->_vote = msg.from();
                }
                else{
                    logger->info("{:d} rejected msgApp [logterm: {:d}, index: {:d}] from {:d}",
                            this->_id, msg.logterm(), msg.index(), msg.from());
                    raftpb::Message tmsg;
                    tmsg.set_to(msg.from());
                    tmsg.set_term(msg.term());
                    tmsg.set_type(raftpb::MsgVoteResp);
                    tmsg.set_reject(true);
                    this->send(tmsg);
                    this->_electionElapsed = 0;
                    this->_vote = msg.from();
                }
                break;
            }
        default:
            try{
                this->_step(this, msg);
            }
            catch(...){
                logger->error("step filed in term {:d}", this->_currentTerm);
                return;
            }
    }
}

void sapphiredb::raft::Raft::addNode(uint64_t to){
    raftpb::Message msg;
    msg.set_type(raftpb::MsgNode);
    msg.set_from(this->_id);
    msg.set_to(to);

    this->send(msg);
}

void sapphiredb::raft::Raft::tickNode(){
    _tick(this);
}

void sapphiredb::raft::Raft::stepNode(raftpb::Message& msg){
    _step(this, msg);
}

void sapphiredb::raft::Raft::stop(){
    this->becomeLocking();
    //TODO unsafety
}

sapphiredb::raft::Raft::Raft(uint64_t id, ::std::string path, uint32_t heartbeatTimeout, uint32_t electionTimeout) :
    _currentTerm(0), _vote(0), _id(id), _leader(0), isLeader(false), _state(sapphiredb::raft::STATE_FOLLOWER), _commitIndex(0), _lastApplied(0),
    _heartbeatElapsed(0), _heartbeatTimeout(heartbeatTimeout), _electionTimeout(electionTimeout), _electionElapsed(0),
    _step(sapphiredb::raft::stepFollower), _tick(sapphiredb::raft::tickElection), _checkQuorum(false){

    try{
        //FILE* log = fopen(path.c_str(), "w");
        //this->logger = spdlog::basic_logger_mt("logger", path);
        this->logger = spdlog::stdout_color_mt("console");

        this->resetRandomizedElectionTimeout();
    }
    catch(...){
        ::std::cout << "some alloc error" << ::std::endl;
    }
}

sapphiredb::raft::Raft::~Raft(){
    try{
        spdlog::drop_all();
    }
    catch(...){
        ::std::abort();
    }
}
