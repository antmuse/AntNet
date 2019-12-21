#include "CQueue2Way.h"
#include "CThread.h"

namespace irr {

CQueue2Way::CQueue2Way() :
    mIdleCount(0),
    mPopCount(0) {
}

CQueue2Way::~CQueue2Way() {
}

CQueue2Way::SNode* CQueue2Way::popNode(bool& iWait, u32 iWaitTime) {
    CQueue2Way::SNode* ret = nullptr;
    iWaitTime /= 10;
    if (0 == iWaitTime) {
        ++iWaitTime;
    }
    for (u32 cnt = 0; iWait && cnt < iWaitTime; ++cnt) {
        mLock.lock();
        if (!mHead.isEmpty()) {
            ret = reinterpret_cast<CQueue2Way::SNode*>(mHead.getNext());
            ret->mLink.delink();
            --mIdleCount;
            ++mPopCount;
            mLock.unlock();
            break;
        }
        mLock.unlock();
        CThread::sleep(10);
    }
    return ret;
}

void CQueue2Way::pushNode(SNode* iNode) {
    if (iNode) {
        //iNode->mLink.pushBack(nullptr);

        CAutoSpinlock ak(mLock);
        ++mIdleCount;
        mHead.pushFront(iNode->mLink);
    }
}

void CQueue2Way::delinkNode(SNode* iNode) {
    if (iNode) {
        CAutoSpinlock ak(mLock);
        iNode->mLink.delink();
        --mIdleCount;
        ++mPopCount;
    }
}

} //namespace irr