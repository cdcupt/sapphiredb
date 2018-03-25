#include "raft/node.h"

sapphiredb::raft::Node::Node(Config& conf, ::std::string log){
    try{
        if(conf.timeout == nullptr && conf.raftlog.empty()){
            this->raft = new Raft(conf.id);
            this->init(conf);
        }
        else{
            this->raft = new Raft(conf.id, conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->heartbeatTimeout);
            this->init(conf);
        }

        this->logger = spdlog::basic_logger_mt("logger", "raft_log");
    }
    catch(...){
        ::std::cerr << "node init failed" << ::std::endl;
    }
}

sapphiredb::raft::Node::Node(Config&& conf, ::std::string log){
    try{
        if(conf.timeout == nullptr && conf.raftlog.empty()){
            this->raft = new Raft(conf.id);
            this->init(conf);
        }
        else{
            this->raft = new Raft(conf.id, conf.raftlog, (conf.timeout)->heartbeatTimeout, (conf.timeout)->heartbeatTimeout);
            this->init(conf);
        }

        this->logger = spdlog::basic_logger_mt("logger", "raft_log");
    }
    catch(...){
        ::std::cerr << "node init failed" << ::std::endl;
    }
}

sapphiredb::raft::Node::~Node(){
    try{
        raft->stop();
        delete raft;
    }
    catch(...){
        ::std::cerr << "free node filed!" << ::std::endl;
    }
}

void sapphiredb::raft::Node::init(Config& conf){
    //lock the node
    this->raft->becomeLocking();
#ifdef LONG_CXX11
    for(auto it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode(*it);
    }
#else
    for(::std::vector<uint64_t>::iterator it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode(*it);
    }
#endif
}

void sapphiredb::raft::Node::init(Config&& conf){
    //lock the node
    this->raft->becomeLocking();
#ifdef LONG_CXX11
    for(auto it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode(*it);
    }
#else
    for(::std::vector<uint64_t>::iterator it = conf.peers.begin(); it!=conf.peers.end(); ++it){
        this->raft->addNode(*it);
    }
#endif
}

void sapphiredb::raft::Node::run(){
    sapphiredb::common::ThreadPool stepTask(1);
    stepTask.enqueue([this](::std::queue<raftpb::Message> recvmsgs) {
            while(!recvmsgs.empty()){
                raft->stepNode(recvmsgs.front());
                recvmsgs.pop();
            }
            }, raft->_recvmsgs);
    for(;;){
        raft->tickNode();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
