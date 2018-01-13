#ifndef APP_CTIMERHEAP_H  
#define APP_CTIMERHEAP_H  

#include "irrTypes.h"


namespace irr {

struct STimerNode {
    STimerNode() :
        mPrevious(0),
        mNext(0) {
    }

    u32 mExpire;
    void(*mCallback)(void*);
    void* mUserPointer;
    STimerNode* mPrevious;
    STimerNode* mNext;
};


class CTimerHeap {
public:
    CTimerHeap();

    ~CTimerHeap();


    void addTimer(STimerNode* iNode);

    ///modify timer
    void setTimer(STimerNode* iNode);


    void removeTimer(STimerNode* iNode);


    void update(u32 iTime);


private:
    void addTimer(STimerNode* iNode, STimerNode* iHeadNode);

    STimerNode* mHead;
    STimerNode* mTail;
};


} //namespace irr

#endif  //APP_CTIMERHEAP_H
