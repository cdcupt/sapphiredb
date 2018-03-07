#ifndef SAPPHIREDB_RAFT_PROGRESS_H_
#define SAPPHIREDB_RAFT_PROGRESS_H_

//namespace sapphiredb
//{
//namespace rafe
//{
typedef ProgressStateType uint64_t;

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
    void resetState(ProgressStateType state);
    void setRecentActive();
    void resetRecentActive();
};
//} //namespace raft
//} //namespace sapphiredb

#endif
