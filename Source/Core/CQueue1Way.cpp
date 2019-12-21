#include "CQueue1Way.h"
#include "CThread.h"

namespace irr {

CQueue1Way::CQueue1Way() :
    mIdleCount(0),
    mPopCount(0),
    mTail(nullptr),
    mHead(nullptr) {
}

CQueue1Way::~CQueue1Way() {
}

CQueue1Way::SNode* CQueue1Way::popNode(u32 iWaitTime) {
    CQueue1Way::SNode* ret = nullptr;
    iWaitTime /= 10;
    if (0 == iWaitTime) {
        ++iWaitTime;
    }
    for (u32 cnt = 0; cnt < iWaitTime; ++cnt) {
        mLock.lock();
        if (mHead) {
            ret = reinterpret_cast<CQueue1Way::SNode*>(mHead);
            if (mHead == mTail) {
                mHead = nullptr;
                mTail = nullptr;
            } else {
                mHead = mHead->getNext();
            }
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

void CQueue1Way::pushNode(SNode* iNode) {
    if (iNode) {
        iNode->mLink.pushBack(nullptr);

        CAutoSpinlock ak(mLock);
        ++mIdleCount;
        if (mTail) {
            mTail->pushBack(iNode->mLink);
        } else {
            mTail = &iNode->mLink;
            mHead = mTail;
        }
    }
}


} //namespace irr