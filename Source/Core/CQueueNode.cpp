#include "CQueueNode.h"
#include <stdlib.h>
#include <string.h>

namespace app {
/////////////////////SQueueRingFreelock/////////////////////
void SQueueRingFreelock::init(s32 size) {
    //APP_ASSERT(size>sizeof(SQueueRingFreelock));

    ::memset(this, 0, sizeof(SQueueRingFreelock));
    mQueueNode.init();
    mSize = size;
}


/////////////////////CQueueNode/////////////////////
CQueueNode* CQueueNode::getNode(const void* value) {
    return  (CQueueNode*) (((s8*) value) - sizeof(CQueueNode));
}

CQueueNode* CQueueNode::createNode(u32 iSize/* = 0*/, AppMallocFunction iMalloc/* = 0*/) {
    iSize += sizeof(CQueueNode);
    CQueueNode* node = (CQueueNode*) (iMalloc ? iMalloc(iSize) : malloc(iSize));
    node->init();
    return node;
}

void CQueueNode::deleteNode(CQueueNode* iNode, AppFreeFunction iFree/* = 0*/) {
    APP_ASSERT(iNode);
    iFree ? iFree((void*) iNode) : free((void*) iNode);
}

void CQueueNode::clear(AppFreeFunction iFree/* = nullptr*/) {
    CQueueNode* node;
    while(!isEmpty()) {
        node = mNext;
        node->delink();
        deleteNode(node, iFree);
    }
}

void CQueueNode::delink() {
    if(mNext) {
        mNext->mPrevious = mPrevious;
    }
    if(mPrevious) {
        mPrevious->mNext = mNext;
    }
    mNext = this;
    mPrevious = this;
}

void CQueueNode::splitAndJoin(CQueueNode& newHead) {
    if(!isEmpty()) {
        CQueueNode* first = mNext;
        CQueueNode* last = mPrevious;
        CQueueNode* at = newHead.getNext();
        first->mPrevious = &newHead;
        newHead.mNext = first;
        last->mNext = at;
        at->mPrevious = last;
    }
    init();
}

void CQueueNode::pushBack(CQueueNode& it) {
    it.mPrevious = mPrevious;
    it.mNext = this;
    mPrevious->mNext = &it;
    mPrevious = &it;
}

void CQueueNode::pushFront(CQueueNode& it) {
    it.mPrevious = this;
    it.mNext = mNext;
    mNext->mPrevious = &it;
    mNext = &it;
}

CQueueNode::CQueueNode() : mNext(this), mPrevious(this) {
}

CQueueNode::~CQueueNode() {
}

CQueueNode* CQueueNode::getNext() const {
    return mNext;
}

CQueueNode* CQueueNode::getPrevious() const {
    return mPrevious;
}

s8* CQueueNode::getValue() const {
    return  ((s8*) this) + sizeof(CQueueNode);
}

void CQueueNode::init() {
    mNext = this;
    mPrevious = this;
}

bool CQueueNode::isEmpty() const {
    return mNext == this;
}

} //namespace app 
