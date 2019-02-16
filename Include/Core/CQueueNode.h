#ifndef APP_CQUEUENODE_H
#define APP_CQUEUENODE_H


#include "HConfig.h"
#include "irrTypes.h"
//#include "HAtomicOperator.h"


namespace irr {

typedef  void* (*AppMallocFunction)(u32);
typedef  void(*AppFreeFunction)(void*);


class CQueueSingleNode {
public:
    APP_FORCE_INLINE static CQueueSingleNode* getNode(const void* value) {
        return  (CQueueSingleNode*) (((s8*) value) - sizeof(CQueueSingleNode));
    }

    static CQueueSingleNode* createNode(u32 iSize = 0, AppMallocFunction iMalloc = 0);

    static void deleteNode(CQueueSingleNode* iNode, AppFreeFunction iFree = 0);


    CQueueSingleNode() : mNext(0) {
    }

    ~CQueueSingleNode() {
    }

    APP_FORCE_INLINE CQueueSingleNode* getNext() const {
        return mNext;
    }

    APP_FORCE_INLINE void pushBack(CQueueSingleNode& it) {
        mNext = &it;
    }

    //APP_FORCE_INLINE void atomicPushBack(CQueueSingleNode* it) {
    //    mNext = &it;
    //}

    APP_FORCE_INLINE void pushBack(CQueueSingleNode* it) {
        mNext = it;
    }

protected:
    CQueueSingleNode* mNext;
};



class CQueueNode {
public:
    APP_FORCE_INLINE static CQueueNode* getNode(const void* value) {
        return  (CQueueNode*) (((s8*) value) - sizeof(CQueueNode));
    }

    static CQueueNode* createNode(u32 iSize = 0, AppMallocFunction iMalloc = 0);

    static void deleteNode(CQueueNode* iNode, AppFreeFunction iFree = 0);


    CQueueNode();
    ~CQueueNode();

    CQueueNode* getNext() const;
    CQueueNode* getPrevious() const;

    c8* getValue() const;

    void init();

    bool isEmpty() const;

    void clear(AppFreeFunction iFree = 0);

    void delink();

    void pushBack(CQueueNode& it);

    void pushBack(CQueueNode* it) {
        pushBack(*it);
    }

    void pushFront(CQueueNode& it);

    void pushFront(CQueueNode* it) {
        pushFront(*it);
    }

    //void delinkBetween(CQueueNode* from, CQueueNode* to) {
    //    to->mPrevious = from;
    //    from->mNext = to;
    //}

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
        EFLS_WRITABLE = 0,                           ///<must be zero, see: SQueueRingFreelock::init();
        EFLS_READABLE = 1
    };
    CQueueNode mQueueNode;
    u32 mCurrentPosition;								///<reading position of this node
    u32 mUserDataSize;									///<user's data size
    u32 mSize;													///<node size
    volatile u8 mFlag;													    ///<User defined flags
    volatile u8 mStatus;													///<node status, see: EFreeLockStatus
    c8 mData[1];												///<node memory

    APP_FORCE_INLINE bool isReadable() {
        return EFLS_READABLE == mStatus;
    }

    APP_FORCE_INLINE bool isWritable() {
        return EFLS_WRITABLE == mStatus;
    }

    APP_FORCE_INLINE void setReadable() {
        mStatus = EFLS_READABLE;
    }

    APP_FORCE_INLINE void setWritable() {
        mStatus = EFLS_WRITABLE;
        mUserDataSize = 0;
        mCurrentPosition = 0;
    }

    void init(s32 size);

    bool isEmpty() const;

    void delink();

    SQueueRingFreelock* getNext() const;

    SQueueRingFreelock* getPrevious() const;

    void pushFront(SQueueRingFreelock* it);

    void pushBack(SQueueRingFreelock* it);
};


} //namespace irr 
#endif //APP_CQUEUENODE_H
