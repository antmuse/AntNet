#include "CTimerHeap.h"  

namespace irr {


CTimerHeap::CTimerHeap() :
    mHead(0),
    mTail(0) {
}


CTimerHeap::~CTimerHeap() {
    STimerNode* tmp = mHead;
    while(tmp) {
        mHead = tmp->mNext;
        delete tmp;
        tmp = mHead;
    }
}


void CTimerHeap::addTimer(STimerNode* iNode) {
    if(!iNode) {
        return;
    }
    if(!mHead) {
        mHead = mTail = iNode;
        return;
    }
    if(iNode->mExpire < mHead->mExpire) {
        iNode->mNext = mHead;
        mHead->mPrevious = iNode;
        mHead = iNode;
        return;
    }
    addTimer(iNode, mHead);
}


void CTimerHeap::setTimer(STimerNode* iNode) {
    if(!iNode) {
        return;
    }
    STimerNode* tmp = iNode->mNext;
    if(!tmp || (iNode->mExpire < tmp->mExpire)) {
        return;
    }
    if(iNode == mHead) {
        mHead = mHead->mNext;
        mHead->mPrevious = 0;
        iNode->mNext = 0;
        addTimer(iNode, mHead);
    } else {
        iNode->mPrevious->mNext = iNode->mNext;
        iNode->mNext->mPrevious = iNode->mPrevious;
        addTimer(iNode, iNode->mNext);
    }
}


void CTimerHeap::removeTimer(STimerNode* iNode) {
    if(!iNode) {
        return;
    }
    if((iNode == mHead) && (iNode == mTail)) {
        delete iNode;
        mHead = 0;
        mTail = 0;
        return;
    }
    if(iNode == mHead) {
        mHead = mHead->mNext;
        mHead->mPrevious = 0;
        delete iNode;
        return;
    }
    if(iNode == mTail) {
        mTail = mTail->mPrevious;
        mTail->mNext = 0;
        delete iNode;
        return;
    }
    iNode->mPrevious->mNext = iNode->mNext;
    iNode->mNext->mPrevious = iNode->mPrevious;
    delete iNode;
}


void CTimerHeap::update(u32 iTime) {
    if(!mHead) {
        return;
    }
    STimerNode* tmp = mHead;
    while(tmp) {
        if(iTime < tmp->mExpire) {
            break;
        }
        tmp->mCallback(tmp->mUserPointer);
        mHead = tmp->mNext;
        if(mHead) {
            mHead->mPrevious = 0;
        }
        delete tmp;
        tmp = mHead;
    }
}


void CTimerHeap::addTimer(STimerNode* iNode, STimerNode* iHeadNode) {
    STimerNode* previous = iHeadNode;
    STimerNode* tmp = previous->mNext;
    while(tmp) {
        if(iNode->mExpire < tmp->mExpire) {
            previous->mNext = iNode;
            iNode->mNext = tmp;
            tmp->mPrevious = iNode;
            iNode->mPrevious = previous;
            break;
        }
        previous = tmp;
        tmp = tmp->mNext;
    }
    if(!tmp) {
        previous->mNext = iNode;
        iNode->mPrevious = previous;
        iNode->mNext = 0;
        mTail = iNode;
    }

}

} //namespace irr

