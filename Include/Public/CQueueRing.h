#ifndef APP_NET_CQUEUERING_H
#define APP_NET_CQUEUERING_H


#include "HConfig.h"
#include "irrTypes.h"


namespace irr {

    typedef  void* (*AppMallocFunction)(s32);
    typedef  void (*AppFreeFunction)(void*);

    class CQueueRing {
    public:
        CQueueRing();
        ~CQueueRing();

        CQueueRing* getNext() const;
        CQueueRing* getPrevious() const;

        s8* getValue() const;

        void init();

        bool isEmpty() const;

        void clear(AppFreeFunction iFree = 0);

        void delink();

        void pushBack(CQueueRing& it);

        void pushBack(CQueueRing* it){
            pushBack(*it);
        }

        void pushFront(CQueueRing& it);

        void pushFront(CQueueRing* it){
            pushFront(*it);
        }

        static CQueueRing* getNode(const void* value);

        static CQueueRing* createNode(u32 iSize = 0, AppMallocFunction iMalloc = 0);

        static void deleteNode(CQueueRing* iNode, AppFreeFunction iFree = 0);

    private:
        CQueueRing* mNext;
        CQueueRing* mPrevious;
    };



    /**
    *@brief A struct used as freelock queue.
    *a wait-free single-producer/single-consumer queue (commonly known as ringbuffer)
    */
    struct SQueueRingFreelock {
        enum EFreeLockStatus{
            EFLS_WRITABLE = 0,                           ///<must be zero, see: SQueueRingFreelock::init();
            EFLS_READABLE = 1
        };
        CQueueRing mQueueNode;
        u32 mCurrentPosition;								///<reading position of this node
        u32 mUserDataSize;									///<user's data size
        u32 mSize;													///<node size
        volatile u8 mFlag;													    ///<User defined flags
        volatile u8 mStatus;													///<node status, see: EFreeLockStatus
        s8 mData[1];												///<node memory

        APP_FORCE_INLINE bool isReadable(){
            return EFLS_READABLE == mStatus;
        }

        APP_FORCE_INLINE bool isWritable(){
            return EFLS_WRITABLE == mStatus;
        }

        APP_FORCE_INLINE void setReadable(){
            mStatus = EFLS_READABLE;
        }

        APP_FORCE_INLINE void setWritable(){
            mStatus = EFLS_WRITABLE;
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
#endif //APP_NET_CQUEUERING_H
