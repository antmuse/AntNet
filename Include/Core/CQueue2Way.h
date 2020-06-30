#pragma once
#include "CSpinlock.h"
#include "CQueueNode.h"

namespace app {
class CQueue2Way {
public:
    struct SNode {
        CQueueNode mLink;
        void* mValue;
    };

    CQueue2Way();
    ~CQueue2Way();

    /**
    * @param iWaitTime max wait time, in milliseconds.
    */
    SNode* popNode(bool& iWait, u32 iWaitTime = 0xFFFFFFFF);

    void pushNode(SNode* iNode);

    void delinkNode(SNode* iNode);

    u32 getPopCount()const {
        return mPopCount;
    }

protected:


private:
    CSpinlock mLock;
    CQueueNode mHead;
    u32 mIdleCount;
    u32 mPopCount;
};

} //namespace app
