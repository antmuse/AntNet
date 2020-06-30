#ifndef APP_CQUEUENODE_H
#define APP_CQUEUENODE_H


#include "HConfig.h"

namespace app {

typedef  void* (*AppMallocFunction)(u32);
typedef  void(*AppFreeFunction)(void*);


class CQueueSingleNode {
public:
    CQueueSingleNode() : mNext(nullptr) {
    }

    CQueueSingleNode* getNext() const {
        return mNext;
    }

    void pushBack(CQueueSingleNode& it) {
        mNext = &it;
    }

    void pushBack(CQueueSingleNode* it) {
        mNext = it;
    }

protected:
    CQueueSingleNode* mNext;
};



class CQueueNode {
public:
    static CQueueNode* getNode(const void* value);

    static CQueueNode* createNode(u32 iSize = 0, AppMallocFunction iMalloc = 0);

    static void deleteNode(CQueueNode* iNode, AppFreeFunction iFree = 0);

    CQueueNode();

    ~CQueueNode();

    CQueueNode* getNext() const;

    CQueueNode* getPrevious() const;

    s8* getValue() const;

    void init();

    bool isEmpty() const;

    void clear(AppFreeFunction iFree = nullptr);

    void delink();

    void pushBack(CQueueNode& it);

    void pushFront(CQueueNode& it);

    /**
    *@brief Split all nodes from this queue and join to the new queue.
    *@param newHead The head node of new queue.
    *@note This is the head of current queue, and head node will not join to new queue.
    */
    void splitAndJoin(CQueueNode& newHead);

protected:
    CQueueNode* mNext;
    CQueueNode* mPrevious;
};



/**
*@brief A struct used as freelock queue.
*a wait-free single-producer/single-consumer queue (commonly known as ringbuffer)
*/
struct SQueueRingFreelock {
    enum EFreeLockStatus {
        EFLS_WRITABLE = 0,                          ///<must be zero, see: SQueueRingFreelock::init();
        EFLS_READABLE = 1
    };
    CQueueNode mQueueNode;
    u32 mCurrentPosition;							///<reading position of this node
    u32 mUserDataSize;								///<user's data size
    u32 mSize;										///<node size
    volatile u8 mFlag;								///<User defined flags
    volatile u8 mStatus;							///<node status, see: EFreeLockStatus
    s8 mData[1];									///<node memory

    bool isReadable()const {
        return EFLS_READABLE == mStatus;
    }

    bool isWritable()const {
        return EFLS_WRITABLE == mStatus;
    }

    void setReadable() {
        mStatus = EFLS_READABLE;
    }

    void setWritable() {
        mUserDataSize = 0;
        mCurrentPosition = 0;
        mStatus = EFLS_WRITABLE;
    }

    void init(s32 size);

    bool isEmpty() const {
        return mQueueNode.isEmpty();
    }

    SQueueRingFreelock* getNext() const {
        return (SQueueRingFreelock*) mQueueNode.getNext();
    }

    SQueueRingFreelock* getPrevious() const {
        return (SQueueRingFreelock*) mQueueNode.getPrevious();
    }

    void delink() {
        mQueueNode.delink();
    }

    void pushBack(SQueueRingFreelock* it) {
        mQueueNode.pushBack(it->mQueueNode);
    }

    void pushFront(SQueueRingFreelock* it) {
        mQueueNode.pushFront(it->mQueueNode);
    }
};


} //namespace app 
#endif //APP_CQUEUENODE_H
