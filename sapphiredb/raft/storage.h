#ifndef SAPPHIREDB_RAFT_STORAGE_H_
#define SAPPHIREDB_RAFT_STORAGE_H_

#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#include "raft/raftpb/raftpb.pb.h"

#define LONG_CXX11

namespace sapphiredb
{
namespace raft
{

class Storage{
private:
    ::std::vector<raftpb::Entry> _entries;
    raftpb::Snapshot* _snap;
    uint64_t _offset;
    ::std::string _path;

public:
    Storage(::std::string path = "./storage.st");
    ~Storage();

    ::std::string serializeData(raftpb::Storage msg);
    ::std::string serializeData(raftpb::HardState msg);
    raftpb::Storage deserializeData(::std::string data);
    raftpb::HardState hdeserializeData(::std::string data);
    uint64_t FirstIndex();
    uint64_t LastIndex();
    uint64_t Term(uint64_t index);
    raftpb::Snapshot Snapshot();
    ::std::vector<raftpb::Entry> Entries();
    void ApplySnapshot(raftpb::Snapshot snap);
    void SetHardState(raftpb::HardState hardstate);
    void Append(::std::vector<raftpb::Entry> ents);
};
} //namespace raft
} //namespace sapphiredb

#endif