#include "CQueueNode.h"
#include <stdlib.h>
#include <string.h>

#include "HMemoryLeakCheck.h"


namespace irr {

CQueueSingleNode* CQueueSingleNode::createNode(u32 iSize, AppMallocFunction iMalloc) {
    iSize += sizeof(CQueueSingleNode);
    CQueueSingleNode* node = (CQueueSingleNode*) (iMalloc ? iMalloc(iSize) : malloc(iSize));
    node->mNext = 0;
    return node;
}

void CQueueSingleNode::deleteNode(CQueueSingleNode* iNode, AppFreeFunction iFree) {
    APP_ASSERT(iNode);
    iFree ? iFree((void*) iNode) : free((void*) iNode);
}






/////////////////////SQueueRingFreelock/////////////////////
void SQueueRingFreelock::init(s32 size) {
    //APP_ASSERT(size>sizeof(SQueueRingFreelock));

    ::memset(this, 0, sizeof(SQueueRingFreelock));
    mQueueNode.init();
    mSize = size;
}


SQueueRingFreelock* SQueueRingFreelock::getNext() const {
    return (SQueueRingFreelock*) mQueueNode.getNext();
}


SQueueRingFreelock* SQueueRingFreelock::getPrevious() const {
    return (SQueueRingFreelock*) mQueueNode.getPrevious();
}


void SQueueRingFreelock::delink() {
    mQueueNode.delink();
}


void SQueueRingFreelock::pushBack(SQueueRingFreelock* it) {
    mQueueNode.pushBack(&it->mQueueNode);
}


void SQueueRingFreelock::pushFront(SQueueRingFreelock* it) {
    mQueueNode.pushFront(&it->mQueueNode);
}



bool SQueueRingFreelock::isEmpty() const {
    return mQueueNode.isEmpty();
}







/////////////////////CQueueNode/////////////////////
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


c8* CQueueNode::getValue() const {
    return  ((c8*) this) + sizeof(CQueueNode);
}


void CQueueNode::init() {
    mNext = this;
    mPrevious = this;
}


bool CQueueNode::isEmpty() const {
    return mNext == this;
}


void CQueueNode::clear(AppFreeFunction iFree /*= 0*/) {
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

/*
void CQueueNode::setAllocator(AppMallocFunction iMalloc, AppFreeFunction iFree){
mMallocHook = iMalloc;
mFreeHook = iFree;
}
*/


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

} //namespace irr 