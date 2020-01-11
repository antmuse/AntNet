#ifndef APP_CBUFFERQUEUE_H
#define APP_CBUFFERQUEUE_H

#include "CQueueNode.h"
#include "CMemoryHub.h"
#include "CSpinlock.h"
#include "INetEventer.h"

namespace irr {
namespace net {

class CEventQueue {
public:
    class CAutolock {
    public:
        CAutolock(CEventQueue& it) : mQueue(it) {
            mQueue.lock();
        }

        ~CAutolock() {
            mQueue.unlock();
        }
    private:
        CEventQueue& mQueue;
    };

    struct SNode {
        CQueueSingleNode mQueueNode;
        CMemoryHub* mMemHub;
        INetEventer* mEventer;
        SNetEvent mEvent;
        void drop() {
            mMemHub->release(this);
        }
    };

    CEventQueue() :
        mMemHub(0),
        mHead(0),
        mTail(0) {

    }

    ~CEventQueue() {
        if(mMemHub) {
            clear();
            mMemHub->drop();
            mMemHub = 0;
        }
    }

    APP_INLINE void lock() {
        mLock.lock();
    }

    APP_INLINE void unlock() {
        mLock.unlock();
    }

    void setMemoryHub(CMemoryHub* it) {
        if(it && it != mMemHub) {
            it->grab();
            if(mMemHub) {
                mMemHub->drop();
            }
            mMemHub = it;
        }
    }

    void clear() {
        CQueueSingleNode* tmp = 0;
        while(mHead) {
            tmp = mHead;
            mHead = mHead->getNext();
            mMemHub->release(tmp);
        }
        mHead = 0;
        mTail = 0;
    }

    void swap(CEventQueue& it) {
        CQueueSingleNode* tmp = mHead;
        mHead = it.mHead;
        it.mHead = tmp;

        tmp = mTail;
        mTail = it.mTail;
        it.mTail = tmp;

        /*CMemoryHub* hub = mMemHub;
        mMemHub = it.mMemHub;
        it.mMemHub = hub;*/
    }

    void push(SNode* it) {
        if(it) {
            APP_ASSERT(0 == it->mQueueNode.getNext());
            it->mQueueNode.pushBack(0);
            if(mTail) {
                mTail->pushBack(it->mQueueNode);
                mTail = mTail->getNext();
            } else {
                mTail = &it->mQueueNode;
                mHead = mTail;
            }
        }
    }

    void push(CEventQueue& it) {
        if(!it.isEmpty()) {
            if(mTail) {
                mTail->pushBack(it.mHead);
                mTail = it.mTail;
            } else {
                mTail = it.mTail;
                mHead = it.mHead;
            }
        }
    }

    void lockPush(SNode* it) {
        CAutoSpinlock ak(mLock);
        push(it);
    }

    bool isEmpty()const {
        return 0 == mHead;
    }

    SNode* pop() {
        CQueueSingleNode* ret = mHead;
        if(ret) {
            mHead = mHead->getNext();
            if(0 == mHead) {
                APP_ASSERT(ret == mTail);
                mTail = 0;
            }
            ret->pushBack(0);
        }
        return reinterpret_cast<SNode*>(ret);
    }

    SNode* lockPop() {
        CAutoSpinlock ak(mLock);
        return pop();
    }

    SNode* lockPick() {
        CAutoSpinlock ak(mLock);
        CQueueSingleNode* ret = mHead;
        return reinterpret_cast<SNode*>(ret);
    }

    SNode* create(u32 size) {
        CMemoryHub* hub = mMemHub;
        SNode* ret = reinterpret_cast<SNode*>(hub->allocate(sizeof(SNode) + size, sizeof(void*)));
        ret->mQueueNode.pushBack(0);
        ret->mMemHub = hub;
        ret->mEvent.mType = ENET_INVALID;
        return ret;
    }

private:
    CMemoryHub* mMemHub;
    CQueueSingleNode* mHead;
    CQueueSingleNode* mTail;
    CSpinlock mLock;
};




class CBufferQueue {
public:
    //8bytes内存对齐, 5*8=40bytes
    struct SBuffer {
        CQueueSingleNode mQueueNode;
        CMemoryHub* mMemHub;
        mutable s32 mReferenceCount; //因原子操作，需保证4bytes内存对齐
        u16 mSessionCount;
        u16 mSessionMax;
        s32 mBufferSize;
        s32 mAllocatedSize;
        c8 mBuffer[1];

        c8* getBuffer()const {
            return const_cast<c8*>(mBuffer) + mSessionMax * sizeof(u32);
        }

        u32 getSessionID(u32 idx)const {
            return *reinterpret_cast<const u32*>(mBuffer + idx * sizeof(u32));
        }

        void grab();

        void drop();
    };

    CBufferQueue();
    ~CBufferQueue();

    SBuffer* getHead() {
        return mHead;
    }

    bool isEmpty()const {
        return 0 == mHead;
    }

    SBuffer* pop();

    SBuffer* lockPop() {
        CAutoSpinlock ak(mLock);
        return pop();
    };

    SBuffer* lockPick();

    SBuffer* createBuffer(u32 size, u16 sessionMax);

    bool lockPush(SBuffer* it);

    bool lockPush(SBuffer& it) {
        return lockPush(&it);
    }

    bool lockPush(const void* buffer, s32 size, const u32 uid) {
        return lockPush(buffer, size, &uid, 1);
    }

    bool lockPush(const void* buffer, s32 size, const u32* uid, u16 maxUID);

    void clear();

    void setMaxCachedSize(s32 bytes) {
        mMaxCachedSize = bytes > 10240 ? bytes : mMaxCachedSize;
    }

    void setMaxEnqueue(s32 cnt) {
        mMaxEnqueue = cnt > 10000 ? cnt : mMaxEnqueue;
    }

    void setMemoryHub(CMemoryHub* it) {
        if(it && it != mMemHub) {
            it->grab();
            if(mMemHub) {
                mMemHub->drop();
            }
            mMemHub = it;
        }
    }


private:
    SBuffer* mHead;
    SBuffer* mTail;
    CMemoryHub* mMemHub;
    CSpinlock mLock;
    s32 mCount;         //count of SBuffer
    s32 mCachedSize;    //current bytes hold in queue.
    s32 mMaxCachedSize; //max bytes hold in queue.
    s32 mMaxEnqueue;    //max buffers hold in queue.
};

}//namespace net
}//namespace irr
#endif //APP_CBUFFERQUEUE_H
