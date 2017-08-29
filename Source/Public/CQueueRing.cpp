#include "CQueueRing.h"
#include <stdlib.h>
#include <string.h>


namespace irr{
    /////////////////////SQueueRingFreelock/////////////////////
    void SQueueRingFreelock::init(s32 size){
        APP_ASSERT(size>sizeof(SQueueRingFreelock));

        memset(this, 0, sizeof(SQueueRingFreelock));
        mQueueNode.init();
        mSize = size;
    }


    SQueueRingFreelock* SQueueRingFreelock::getNext() const{
        return (SQueueRingFreelock*) mQueueNode.getNext();
    }


    SQueueRingFreelock* SQueueRingFreelock::getPrevious() const{
        return (SQueueRingFreelock*) mQueueNode.getPrevious();
    }


    void SQueueRingFreelock::delink(){
        mQueueNode.delink();
    }


    void SQueueRingFreelock::pushBack(SQueueRingFreelock* it) {
        mQueueNode.pushBack(&it->mQueueNode);
    }


    void SQueueRingFreelock::pushFront(SQueueRingFreelock* it) {
        mQueueNode.pushFront(&it->mQueueNode);
    }



    bool SQueueRingFreelock::isEmpty() const{
        return mQueueNode.isEmpty();
    }







    /////////////////////CQueueRing/////////////////////
    CQueueRing::CQueueRing() : mNext(this), mPrevious(this){
    }


    CQueueRing::~CQueueRing(){
    }


    CQueueRing* CQueueRing::getNext() const{
        return mNext;
    }


    CQueueRing* CQueueRing::getPrevious() const{
        return mPrevious;
    }


    CQueueRing* CQueueRing::getNode(const void* value){
        return  (CQueueRing*)(((s8*)value) - sizeof(CQueueRing));
    }


    s8* CQueueRing::getValue() const{
        return  ((s8*)this) + sizeof(CQueueRing);
    }


    void CQueueRing::init(){
        mNext = this;
        mPrevious = this;
    }


    bool CQueueRing::isEmpty() const{
        return mNext == this;
    }


    void CQueueRing::clear(AppFreeFunction iFree /*= 0*/){
        CQueueRing* node;
        while(!isEmpty()){
            node = mNext;
            node->delink();
            deleteNode(node, iFree);
        }
    }


    void CQueueRing::delink(){
        if(mNext){
            mNext->mPrevious = mPrevious;
        }
        if(mPrevious){
            mPrevious->mNext = mNext;
        }
        mNext = this;
        mPrevious = this;
    }


    void CQueueRing::pushBack(CQueueRing& it){
        it.mPrevious = mPrevious;
        it.mNext = this;
        mPrevious->mNext = &it;
        mPrevious = &it;
    }


    void CQueueRing::pushFront(CQueueRing& it){
        it.mPrevious = this;
        it.mNext = mNext;
        mNext->mPrevious = &it;
        mNext = &it;
    }

    /*
    void CQueueRing::setAllocator(AppMallocFunction iMalloc, AppFreeFunction iFree){
    mMallocHook = iMalloc;
    mFreeHook = iFree;
    }
    */


    CQueueRing* CQueueRing::createNode(u32 iSize/* = 0*/, AppMallocFunction iMalloc/* = 0*/){
        iSize += sizeof(CQueueRing);
        CQueueRing* node = (CQueueRing*)(iMalloc ? iMalloc(iSize) : malloc(iSize));
        node->init();
        return node;
    }


    void CQueueRing::deleteNode(CQueueRing* iNode, AppFreeFunction iFree/* = 0*/){
        APP_ASSERT(iNode);
        iFree ? iFree((void*)iNode) : free((void*)iNode);
    }

} //namespace irr 