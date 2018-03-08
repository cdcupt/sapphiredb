#include "progress.h"

void Progress::resetState(ProgressStateType state){
    this._pendingSnapshot = 0;
    this._state = state;
}

void Progress::becomeProbe(){
    if(this._state == ProgressStateSnapshot){
        uint64_t tmpPendingSnapshot = this._pendingSnapshot;
        this.resetState(ProgressStateProbe);
        this._next = max(this._match+1, tmpPendingSnapshot+1);
    }
    else{
        this.resetState(ProgressStateProbe);
        this._next = this._match+1;
    }
}

void Progress::becomeReplicate(){
    this.resetState(ProgressStateReplicate);
    this._next = this._match+1;
}

void Progress::becomeSnapshot(uint64_t snapshoti){
    this.resetState(ProgressStateSnapshot);
    this._pendingSnapshot = snapshoti;
}

void Progress::setRecentActive(){
    this._recentActive = true;
}

void Progress::resetRecentActive(){
    this._recentActive = false;
}

uint64_t Progress::getMatch(){
    return this._match;
}

uint64_t Progress::getNext(){
    return this._next;
}
