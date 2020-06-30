#include "CBufferQueue.h"
#include "CMemoryHub.h"
#include "HAtomicOperator.h"

namespace app {
namespace net {


void CBufferQueue::SBuffer::grab() {
    APP_ASSERT(mReferenceCount > 0);
    AppAtomicIncrementFetch(&mReferenceCount);
}

void CBufferQueue::SBuffer::drop() {
    s32 ret = AppAtomicDecrementFetch(&mReferenceCount);
    APP_ASSERT(ret >= 0 && mMemHub);
    if(0 == ret) {
        mMemHub->release(this);
    }
}


//////////////////////////////////////////////////////////////////////////////////////
CBufferQueue::CBufferQueue() :
    mCount(0),
    mCachedSize(0),
    mMaxCachedSize(1024 * 1024 * 1024),
    mMaxEnqueue(100000),
    mMemHub(0),
    mHead(0),
    mTail(0) {
}


CBufferQueue::~CBufferQueue() {
    if(mMemHub) {
        clear();
        mMemHub->drop();
        mMemHub = 0;
    }
}


void CBufferQueue::clear() {
    CBufferQueue::SBuffer* tmp = 0;
    while(mHead) {
        tmp = mHead;
        mHead = reinterpret_cast<SBuffer*>(mHead->mQueueNode.getNext());
        mMemHub->release(tmp);
    }
    mHead = 0;
    mTail = 0;
}

CBufferQueue::SBuffer* CBufferQueue::lockPick() {
    CAutoSpinlock ak(mLock);
    CBufferQueue::SBuffer* ret = mHead;
    return ret;
}


CBufferQueue::SBuffer* CBufferQueue::pop() {
    CBufferQueue::SBuffer* ret = mHead;
    if(ret) {
        mHead = reinterpret_cast<SBuffer*>(mHead->mQueueNode.getNext());
        if(0 == mHead) {
            APP_ASSERT(ret == mTail);
            mTail = 0;
        }
        ret->mQueueNode.pushBack(0);
        AppAtomicFetchAdd(-ret->mBufferSize, &mCachedSize);
        AppAtomicDecrementFetch(&mCount);
        //APP_ASSERT(ret->mBufferSize == (*((u32*) (ret->mBuffer)+ret->mSessionMax)));
    }
    return ret;
}


CBufferQueue::SBuffer* CBufferQueue::createBuffer(u32 size, u16 sessionMax) {
    CMemoryHub* hub = mMemHub;
    SBuffer* buf = reinterpret_cast<SBuffer*>(
        hub->allocate(size + sizeof(SBuffer) + sessionMax * sizeof(u32) - sizeof(SBuffer::mBuffer),
        sizeof(void*))
        );
    buf->mReferenceCount = sessionMax;//grab for all session
    buf->mSessionCount = 0;
    buf->mSessionMax = sessionMax;
    buf->mQueueNode.pushBack(0);
    buf->mMemHub = hub;
    buf->mAllocatedSize = size;
    buf->mBufferSize = 0;
    return buf;
}


bool CBufferQueue::lockPush(CBufferQueue::SBuffer* it) {
    if(!it || mCachedSize + it->mBufferSize >= mMaxCachedSize || mCount >= mMaxEnqueue) {
        return false;
    }
    //APP_ASSERT(it->mBufferSize == (*((u32*) (it->mBuffer) + it->mSessionMax)));
    APP_ASSERT(0 == it->mQueueNode.getNext());
    it->mQueueNode.pushBack(0);

    AppAtomicFetchAdd(it->mBufferSize, &mCachedSize);
    AppAtomicIncrementFetch(&mCount);

    CAutoSpinlock alk(mLock);
    if(mTail) {
        mTail->mQueueNode.pushBack(it->mQueueNode);
        mTail = it;
    } else {
        mTail = it;
        mHead = it;
    }
    return true;
}


bool CBufferQueue::lockPush(const void* buffer, s32 size, const u32* uid, u16 maxUID) {
    if(!buffer || size <= 0 || mCachedSize + size >= mMaxCachedSize || mCount >= mMaxEnqueue) {
        return false;
    }
    AppAtomicFetchAdd((s32) size, &mCachedSize);
    AppAtomicIncrementFetch(&mCount);
    SBuffer* buf = createBuffer(size, maxUID);
    u32* id = reinterpret_cast<u32*>(buf->mBuffer);
    for(u16 i = 0; i < maxUID; ++i) {
        *id++ = uid[i];
    }
    ::memcpy(id, buffer, size);
    buf->mBufferSize = size;
    //APP_ASSERT(buf->mBufferSize == (*((u32*) (buf->mBuffer) + buf->mSessionMax)));

    CAutoSpinlock alk(mLock);
    if(mTail) {
        mTail->mQueueNode.pushBack(buf->mQueueNode);
        mTail = buf;
    } else {
        mTail = buf;
        mHead = buf;
    }
    return true;
}


}//namespace net
}//namespace app
