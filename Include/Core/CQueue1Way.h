#pragma once
#include "CSpinlock.h"
#include "CQueueNode.h"

namespace irr {
class CQueue1Way {
public:
    struct SNode {
        CQueueSingleNode mLink;
        void* mValue;
    };

    CQueue1Way();
    ~CQueue1Way();

    /**
    * @param iWaitTime max wait time, in milliseconds.
    */
    SNode* popNode(u32 iWaitTime = 0xFFFFFFFF);

    void pushNode(SNode* iNode);

protected:


private:
    CSpinlock mLock;
    CQueueSingleNode* mTail;
    CQueueSingleNode* mHead;
    u32 mIdleCount;
    u32 mPopCount;
};

} //namespace irr
