#include "raft/storage.h"

sapphiredb::raft::Storage::Storage(::std::string path) : _path(path){
    
}

sapphiredb::raft::Storage::~Storage(){

}

::std::string sapphiredb::raft::Storage::serializeData(raftpb::Storage msg){
    ::std::string data;
    msg.SerializeToString(&data);
    return data;
}

raftpb::Storage sapphiredb::raft::Storage::deserializeData(::std::string data){
    raftpb::Storage msg;
    msg.ParseFromString(data);
    return msg;
}

::std::string sapphiredb::raft::Storage::serializeData(raftpb::HardState msg){
    ::std::string data;
    msg.SerializeToString(&data);
    return data;
}

raftpb::HardState sapphiredb::raft::Storage::hdeserializeData(::std::string data){
    raftpb::HardState msg;
    msg.ParseFromString(data);
    return msg;
}

uint64_t sapphiredb::raft::Storage::FirstIndex(){
    ::std::ifstream in(_path, ::std::ios::in);
    if (in.is_open())
    {
        //::std::string data;
        //in >> data;
        char rdata[1024];
        in.read(rdata, 1024);
        ::std::string data(rdata);
        raftpb::Storage msg = deserializeData(data);
        in.close();
        return msg.snap().metadata().index();
    }

    return 0;
}

uint64_t sapphiredb::raft::Storage::LastIndex(){
    ::std::ifstream in(_path, ::std::ios::in);
    if (in.is_open())   
    {
        //::std::string data;
        //in >> data;
        char rdata[1024];
        in.read(rdata, 1024);
        ::std::string data(rdata);
        raftpb::Storage msg = deserializeData(data);
        in.close();
        uint64_t l = msg.entries().size();
        if(l > 0) return msg.offset()+l-1;
        return msg.snap().metadata().index();
    }

    return 0;
}

uint64_t sapphiredb::raft::Storage::Term(uint64_t index){
    ::std::ifstream in(_path, ::std::ios::in);
    if (in.is_open())   
    {
        //::std::string data;
        //in >> data;
        char rdata[1024];
        in.read(rdata, 1024);
        ::std::string data(rdata);
        raftpb::Storage msg = deserializeData(data);
        in.close();
        ::std::cout << "index: " << index << " msg.offset(): " << msg.offset() << ::std::endl;
        if(index < msg.offset()){
            if(msg.snap().metadata().index() == index){
                return msg.snap().metadata().term();
            }
            return 0;
        }

        uint64_t last = LastIndex();
        ::std::cout << "index: " << index << " last: " << last << " msg.offset(): " << msg.offset() << ::std::endl;
        if(last <= 0){
            return 0;
        }
        if(index > last){
            return 0;
        }

        return msg.entries(index-msg.offset()).term();
    }

    return 0;
}

raftpb::Snapshot sapphiredb::raft::Storage::Snapshot(){
    ::std::ifstream in(_path, ::std::ios::in);
    if (in.is_open())   
    {
        //::std::string data;
        //in >> data;
        char rdata[1024];
        in.read(rdata, 1024);
        ::std::string data(rdata);
        raftpb::Storage msg = deserializeData(data);
        in.close();
        return msg.snap();
    }

    return raftpb::Snapshot();
}

::std::vector<raftpb::Entry> sapphiredb::raft::Storage::Entries(){
    ::std::ifstream in(_path, ::std::ios::in);
    if (in.is_open())   
    {
        char rdata[1024];
        in.read(rdata, 1024);
        ::std::string data(rdata);
        raftpb::Storage msg = deserializeData(data);
        in.close();
        ::std::vector<raftpb::Entry> ents;
        for(int i=0; i<msg.entries_size(); ++i){
            ents.push_back(msg.entries(i));
        }
        return ents;
    }

    return ::std::vector<raftpb::Entry>();
}

void sapphiredb::raft::Storage::ApplySnapshot(raftpb::Snapshot snap){
    ::std::ifstream fin(_path);
    if (!fin){
        ::std::ofstream file(_path, ::std::ios::trunc|::std::ios::binary);
        raftpb::Storage msg;
        msg.mutable_snap()->set_data(snap.data());
        for(int i=0; i<snap.metadata().conf_state().nodes_size(); ++i){
            msg.mutable_snap()->mutable_metadata()->mutable_conf_state()->add_nodes(snap.metadata().conf_state().nodes(i));
            msg.mutable_snap()->mutable_metadata()->mutable_conf_state()->add_learners(snap.metadata().conf_state().learners(i));
        }
        msg.mutable_snap()->mutable_metadata()->set_index(snap.metadata().index());
        msg.mutable_snap()->mutable_metadata()->set_term(snap.metadata().term());
        ::std::string data = serializeData(msg);
        //file << data;
        file.write(data.c_str(), data.size());
        file.close();
    }
    else{
        //::std::string rdata;
        //fin >> rdata;
        char rdata[1024];
        fin.read(rdata, 1024);
        ::std::string sdata(rdata);
        raftpb::Storage msg = deserializeData(sdata);
        fin.close();

        ::std::ofstream file(_path, ::std::ios::trunc|::std::ios::binary);
        msg.mutable_snap()->set_data(snap.data());
        for(int i=0; i<snap.metadata().conf_state().nodes_size(); ++i){
            msg.mutable_snap()->mutable_metadata()->mutable_conf_state()->add_nodes(snap.metadata().conf_state().nodes(i));
            msg.mutable_snap()->mutable_metadata()->mutable_conf_state()->add_learners(snap.metadata().conf_state().learners(i));
        }
        msg.mutable_snap()->mutable_metadata()->set_index(snap.metadata().index());
        msg.mutable_snap()->mutable_metadata()->set_term(snap.metadata().term());
        ::std::string data = serializeData(msg);
        //file << data;
        file.write(data.c_str(), data.size());
        file.close();
    }
}

void sapphiredb::raft::Storage::SetHardState(raftpb::HardState hardstate){
    ::std::ifstream fin(_path);
    if (!fin){
        ::std::ofstream file(_path, ::std::ios::trunc|::std::ios::binary);
        raftpb::HardState msg;
        msg.set_term(hardstate.term());
        msg.set_term(hardstate.vote());
        msg.set_term(hardstate.commit());
        ::std::string data = serializeData(msg);
        //file << data;
        file.write(data.c_str(), data.size());
        file.close();
    }
    else{
        //::std::string rdata;
        //fin >> rdata;
        char rdata[1024];
        fin.read(rdata, 1024);
        ::std::string sdata(rdata);
        raftpb::HardState msg = hdeserializeData(sdata);
        fin.close();

        ::std::ofstream file(_path, ::std::ios::trunc|::std::ios::binary);
        msg.set_term(hardstate.term());
        msg.set_term(hardstate.vote());
        msg.set_term(hardstate.commit());
        ::std::string data = serializeData(msg);
        //file << data;
        file.write(data.c_str(), data.size());
        file.close();
    }
}

void sapphiredb::raft::Storage::Append(::std::vector<raftpb::Entry> ents){
    ::std::ifstream fin(_path, ::std::ios::binary);
    if (!fin){
        ::std::ofstream file(_path, ::std::ios::trunc|::std::ios::binary);
        raftpb::Storage msg;
        for(int i=0; i<ents.size(); ++i){
            raftpb::Entry* entry = msg.add_entries();
            entry->set_type(ents[i].type());
            entry->set_term(ents[i].term());
            entry->set_index(ents[i].index());
            entry->set_data(ents[i].data());
        }
        ::std::string data = serializeData(msg);


        raftpb::Storage tmsg = deserializeData(data);
        ::std::vector<raftpb::Entry> tents;
        for(int i=0; i<tmsg.entries_size(); ++i){
            tents.push_back(tmsg.entries(i));
        }
        ::std::cout << "tents.size(): " << tents.size() << ::std::endl;
        for(int i=0; i<tents.size(); ++i){
            ::std::cout << "*********entry->set_data(tents[i].data()): " << tents[i].data() << ::std::endl;
        }



        //file << data;
        file.write(data.c_str(), data.size());
        file.close();
    }
    else{
        char rdata[1024];
        fin.read(rdata, 1024);
        ::std::string sdata(rdata);
        raftpb::Storage msg = deserializeData(sdata);
        fin.close();

        ::std::ofstream file(_path, ::std::ios::trunc|::std::ios::binary);
        for(int i=0; i<ents.size(); ++i){
            raftpb::Entry* entry = msg.add_entries();
            entry->set_type(ents[i].type());
            entry->set_term(ents[i].term());
            entry->set_index(ents[i].index());
            entry->set_data(ents[i].data());
        }
        ::std::string data = serializeData(msg);


        raftpb::Storage tmsg = deserializeData(data);
        ::std::vector<raftpb::Entry> tents;
        for(int i=0; i<tmsg.entries_size(); ++i){
            tents.push_back(tmsg.entries(i));
        }
        ::std::cout << "tents.size(): " << tents.size() << ::std::endl;
        for(int i=0; i<tents.size(); ++i){
            ::std::cout << "*********entry->set_data(tents[i].data()): " << tents[i].data() << ::std::endl;
        }


        //file << data;
        file.write(data.c_str(), data.size());
        file.close();
    }
}
//TODO wal