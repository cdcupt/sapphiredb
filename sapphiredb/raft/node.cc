#include "raft/node.h"

sapphiredb::raft::Timeout::Timeout(uint64_t heartbeatTimeout, uint64_t elecctionTimeout){
    this->heartbeatTimeout = heartbeatTimeout;
    this->elecctionTimeout = elecctionTimeout;
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
        this->kque = new sapphiredb::common::Kqueue(conf.socket.first, conf.socket.second, sapphiredb::common::Netcon::IPV4, 1024, 1024, 100);
        this->logger = spdlog::stdout_color_mt("node_console");
        //this->logger = spdlog::basic_logger_mt("logger", "raft_log");
        //uid = &common::Uniqueid::Instance();
        if(conf.timeout == nullptr && conf.raftlog.empty()){
            if(conf.id != 0) this->raft = new Raft(conf.id);
            //else this->raft = new Raft(common::Uniqueid::Instance().getUniqueid());
            this->init(conf);
        }
        else{
            if(conf.id != 0) this->raft = new Raft(conf.id, conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->heartbeatTimeout);
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
        this->kque = new sapphiredb::common::Kqueue(conf.socket.first, conf.socket.second, sapphiredb::common::Netcon::IPV4, 1024, 1024, 100);
        this->logger = spdlog::stdout_color_mt("node_console");
        //this->logger = spdlog::basic_logger_mt("logger", "raft_log");
        //uid = &common::Uniqueid::Instance();
        if(conf.timeout == nullptr && conf.raftlog.empty()){
            if(conf.id != 0) this->raft = new Raft(conf.id);
            //else this->raft = new Raft(common::Uniqueid::Instance().getUniqueid());
            this->init(conf);
        }
        else{
            if(conf.id != 0) this->raft = new Raft(conf.id, conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->heartbeatTimeout);
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
        raft->stop();
        delete uid;
        delete raft;
    }
    catch(...){
        ::std::cerr << "free node filed!" << ::std::endl;
    }
}

void sapphiredb::raft::Node::init(Config& conf){
    //lock the node
    this->raft->becomeLocking();
    this->kque->listenp();
#ifdef LONG_CXX11
    for(auto it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode((*it).first);
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
    }
#else
    for(::std::vector<::std::pair<uint64_t, ::std::pair<::std::string, uint32_t>>>::iterator it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode((*it).first);
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
    }
#endif

    sapphiredb::common::ThreadPool epoll_loop(1); //epoll loop thread
    epoll_loop.enqueue([this](){
        while(1){
            this->kque->loop_once(1000);
        }
    });

    sapphiredb::common::ThreadPool trecv(1);
    trecv.enqueue([this](){
        while(1){
            //TODO thread condition
            this->raft->tryPushRecvbuf(this->kque->popData());
        }
    });

    sapphiredb::common::ThreadPool tsend(1);
    tsend.enqueue([this](){
        while(1){
            //TODO thread condition
            this->kque->pushData(this->raft->tryPopSendbuf());
        }
    });
}

void sapphiredb::raft::Node::init(Config&& conf){
    //lock the node
    this->raft->becomeLocking();
    this->kque->listenp();
#ifdef LONG_CXX11
    for(auto it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode((*it).first);
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
    }
#else
    for(::std::vector<uint64_t>::iterator it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode((*it).first);
        this->kque->conn(::std::move((*it).second.first), (*it).second.second, (*it).first);
    }
#endif

    sapphiredb::common::ThreadPool epoll_loop(1); //epoll loop thread
    epoll_loop.enqueue([this](){
        while(1){
            this->kque->loop_once(1000);
        }
    });

    sapphiredb::common::ThreadPool trecv(1);
    trecv.enqueue([this](){
        while(1){
            //TODO thread condition
            this->raft->tryPushRecvbuf(this->kque->popData());
        }
    });

    sapphiredb::common::ThreadPool tsend(1);
    tsend.enqueue([this](){
        while(1){
            //TODO thread condition
            this->kque->pushData(this->raft->tryPopSendbuf());
        }
    });
}

void sapphiredb::raft::Node::run(){
    sapphiredb::common::ThreadPool stepTask(1);
    stepTask.enqueue([this]() {
            while(1){
                raft->stepNode();
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
        raft->tickNode();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
