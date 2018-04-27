#include "raft/node.h"

sapphiredb::raft::Timeout::Timeout(uint64_t heartbeatTimeout, uint64_t electionTimeout){
    this->heartbeatTimeout = heartbeatTimeout;
    this->electionTimeout = electionTimeout;
}

sapphiredb::raft::Config::Config(uint64_t id, ::std::string raftlog, Timeout* timeout, ::std::pair<::std::string, uint32_t> socket, ::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>> peers){
    this->id = id;
    this->raftlog = raftlog;
    this->timeout = timeout;
    this->socket = socket;
    this->peers = peers;
}

sapphiredb::raft::Config::~Config(){
    delete timeout;
}

sapphiredb::raft::Node::Node(Config& conf, ::std::string log){
    try{
        this->kque = new sapphiredb::common::Kqueue(conf.socket.first, conf.socket.second, sapphiredb::common::Netcon::IPV4, 1024, 1024, 100, &(this->tsend_condition), &(this->trecv_condition), &(this->tbind_condition), &(this->tstep_condition));
        this->logger = spdlog::stdout_color_mt("node_console");
        //this->logger = spdlog::basic_logger_mt("logger", "raft_log");
        //uid = &common::Uniqueid::Instance();
        if(conf.timeout == nullptr && conf.raftlog.empty()){
            if(conf.id != 0) this->raft = new Raft(conf.id, &(this->tsend_condition), &(this->trecv_condition), &(this->tbind_condition), &(this->tstep_condition));
            //else this->raft = new Raft(common::Uniqueid::Instance().getUniqueid());
            this->init(conf);
        }
        else{
            if(conf.id != 0) this->raft = new Raft(conf.id, &(this->tsend_condition), &(this->trecv_condition), &(this->tbind_condition), &(this->tstep_condition), conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->electionTimeout);
            //else this->raft = new Raft(common::Uniqueid::Instance().getUniqueid(), conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->heartbeatTimeout);
            this->init(conf);
        }
    }
    catch(...){
        ::std::cerr << "node init failed" << ::std::endl;
    }
}

sapphiredb::raft::Node::Node(Config&& conf, ::std::string log){
    try{
        this->kque = new sapphiredb::common::Kqueue(conf.socket.first, conf.socket.second, sapphiredb::common::Netcon::IPV4, 1024, 1024, 100, &(this->tsend_condition), &(this->trecv_condition), &(this->tbind_condition), &(this->tstep_condition));
        this->logger = spdlog::stdout_color_mt("node_console");
        //this->logger = spdlog::basic_logger_mt("logger", "raft_log");
        //uid = &common::Uniqueid::Instance();
        if(conf.timeout == nullptr && conf.raftlog.empty()){
            /*if(conf.id != 0)*/ this->raft = new Raft(conf.id, &(this->tsend_condition), &(this->trecv_condition), &(this->tbind_condition), &(this->tstep_condition));
            //else this->raft = new Raft(common::Uniqueid::Instance().getUniqueid());
            this->init(conf);
        }
        else{
            /*if(conf.id != 0)*/ this->raft = new Raft(conf.id, &(this->tsend_condition), &(this->trecv_condition), &(this->tbind_condition), &(this->tstep_condition), conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->electionTimeout);
            //else this->raft = new Raft(common::Uniqueid::Instance().getUniqueid(), conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->heartbeatTimeout);
            this->init(conf);
        }
    }
    catch(...){
        ::std::cerr << "node init failed" << ::std::endl;
    }
}

sapphiredb::raft::Node::~Node(){
    try{
        raft->stop();//nothing to do
        delete uid;
        delete raft;
    }
    catch(...){
        ::std::cerr << "free node filed!" << ::std::endl;
    }
}

void sapphiredb::raft::Node::init(Config& conf){
    //add new node
    this->kque->listenp();
    this->raft->stepDown(1, 0);
#ifdef LONG_CXX11
    for(auto it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
        this->raft->addNode((*it).first);
        this->raft->sendAddNode((*it).first);
    }
#else
    for(::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>>::iterator it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
        this->raft->addNode((*it).first);
        this->raft->sendAddNode((*it).first);
    }
#endif

    static sapphiredb::common::ThreadPool epoll_loop(1); //epoll loop thread
    epoll_loop.enqueue([this](){
        while(1){
            this->kque->loop_once(1000);
        }
    });

    static sapphiredb::common::ThreadPool trecv(1);
    trecv.enqueue([this](){
        while(1){
            ::std::unique_lock<std::mutex> lock(this->trecv_mutex);
            this->trecv_condition.wait(lock, [this]{ return !this->kque->isRecvEmpty(); });

            if(this->raft->tryPushRecvbuf(this->kque->popData())){
                this->tsend_condition.notify_one();
            }
        }
    });

    static sapphiredb::common::ThreadPool tsend(1);
    tsend.enqueue([this, &conf](){
        while(1){
            ::std::unique_lock<std::mutex> lock(this->tsend_mutex);
            this->tsend_condition.wait(lock, [this]{ return !this->raft->isSendEmpty(); });

            sapphiredb::raft::Sendstruct tsendstruct = this->raft->tryPopSendbuf();
            ::std::string msg = tsendstruct.msg;
            uint64_t to = tsendstruct.to;
            
            if(!msg.empty()){
                this->kque->funcPeerfd([this, &msg, &to](::std::unordered_map<uint64_t, int32_t> peersfd){
                    if(to == 0){
                        for(::std::unordered_map<uint64_t, int32_t>::iterator it = peersfd.begin(); it != peersfd.end(); ++it){
                            while(!this->kque->pushData(msg));
                            this->kque->send((*it).first);
                        }
                    }
                    else{
                        while(!this->kque->pushData(msg));
                        this->kque->send(to);
                    }
                });
            }
        }
    });
}

void sapphiredb::raft::Node::init(Config&& conf){
    //add new node
    this->kque->listenp();
    this->raft->stepDown(1, 0);
#ifdef LONG_CXX11
    for(auto it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
        this->raft->addNode((*it).first);
        this->raft->sendAddNode((*it).first);
    }
#else
    for(::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>>::iterator it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
        this->raft->addNode((*it).first);
        this->raft->sendAddNode((*it).first);
    }
#endif

    static sapphiredb::common::ThreadPool epoll_loop(1); //epoll loop thread
    epoll_loop.enqueue([this](){
        while(1){
            this->kque->loop_once(1000);
        }
    });

    static sapphiredb::common::ThreadPool trecv(1);
    trecv.enqueue([this](){
        while(1){
            ::std::unique_lock<std::mutex> lock(this->trecv_mutex);
            this->trecv_condition.wait(lock, [this]{ return this->raft->isRecvEmpty(); });
            if(this->raft->tryPushRecvbuf(this->kque->popData())){
                this->tsend_condition.notify_one();
            }
        }
    });

    static sapphiredb::common::ThreadPool tsend(1);
    tsend.enqueue([this, &conf](){
        while(1){
            ::std::unique_lock<std::mutex> lock(this->tsend_mutex);
            this->tsend_condition.wait(lock, [this]{ return !this->raft->isSendEmpty(); });

            sapphiredb::raft::Sendstruct tsendstruct = this->raft->tryPopSendbuf();
            ::std::string msg = tsendstruct.msg;
            uint64_t to = tsendstruct.to;
            
            if(!msg.empty()){
                this->kque->funcPeerfd([this, &msg, &to](::std::unordered_map<uint64_t, int32_t> peersfd){
                    if(to == 0){
                        for(::std::unordered_map<uint64_t, int32_t>::iterator it = peersfd.begin(); it != peersfd.end(); ++it){
                            while(!this->kque->pushData(msg));
                            this->kque->send((*it).first);
                        }
                    }
                    else{
                        while(!this->kque->pushData(msg));
                        this->kque->send(to);
                    }
                });
            }
        }
    });
}

void sapphiredb::raft::Node::run(){
    static sapphiredb::common::ThreadPool bindTask(1);
    bindTask.enqueue([this]() {
        while(1){
            ::std::unique_lock<std::mutex> lock(this->tbind_mutex);
            this->tbind_condition.wait(lock, [this]{ return !this->kque->emptyUnknownfd() && !this->raft->emptyUnknownid(); });

            while(!this->kque->emptyUnknownfd() && !this->raft->emptyUnknownid()){
                uint64_t id = this->raft->popUnknownid();
                int32_t fd = this->kque->popUnknownfd();
                this->kque->bindPeerfd(id, fd);
                logger->error("id: {:d} | fd: {:d}", id, fd);
            }
        }
    });

    static sapphiredb::common::ThreadPool stepTask(1);
    stepTask.enqueue([this]() {
        while(1){
            ::std::unique_lock<std::mutex> lock(this->tstep_mutex);
            this->tstep_condition.wait(lock, [this]{ return !this->raft->isRecvEmpty(); });

            this->raft->stepNode();
        }
    });
    /*
    kv.enqueue([this]() {
            while(1){
                //TODO send append log msg
            }
        });
    */
    for(;;){
        this->raft->tickNode(this->raft);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


//TODO mutli send heartbeat to peer about progress struct