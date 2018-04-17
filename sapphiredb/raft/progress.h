#ifndef SAPPHIREDB_RAFT_PROGRESS_H_
#define SAPPHIREDB_RAFT_PROGRESS_H_

#include <iostream>

namespace sapphiredb
{
namespace raft
{
typedef uint64_t ProgressStateType;

enum{
    ProgressStateSnapshot = 1,
    ProgressStateProbe,
    ProgressStateReplicate
};

class Progress{
private:
    uint64_t _next;
    uint64_t _match;

    ProgressStateType _state;

    //If there is a pending snapshot, the pendingSnapshot will be set to the index of the snapshot.
    uint64_t _pendingSnapshot;

    bool _recentActive;

    bool _isLeader;
public:
    ProgressStateType getState();
    void resetState(ProgressStateType state);
    void setRecentActive();
    void resetRecentActive();
    uint64_t getMatch();
    uint64_t getNext();
    void optimisticUpdate(uint64_t n);

    void becomeProbe();
    void becomeReplicate();
    void becomeSnapshot(uint64_t snapshoti);

    inline static ::std::string name(ProgressStateType e){
        switch(e){
            case ProgressStateSnapshot: return "ProgressStateSnapshot";
            case ProgressStateProbe: return "ProgressStateProbe";
            case ProgressStateReplicate: return "ProgressStateReplicate";
        }
    }
};
} //namespace raft
} //namespace sapphiredb

#endif
