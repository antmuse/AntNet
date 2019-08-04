#include "CNetSessionPool.h"
//#include "IAppLogger.h"

namespace irr {
namespace net {

CNetSessionPool::CNetSessionPool() :
    mIdle(0),
    mMax(0),
    mClosed(false),
    mHead(0),
    mTail(0) {
    mAllContext = new CNetSession*[ENET_SESSION_MASK];
    ::memset(mAllContext, 0, ENET_SESSION_MASK * sizeof(CNetSession*));
}

CNetSessionPool::~CNetSessionPool() {
    clearAll();
    delete mAllContext;
}

CNetSession* CNetSessionPool::createSession() {
    APP_ASSERT(0 == mAllContext[mMax]);
    if(mMax < ENET_SESSION_MASK) {
        mAllContext[mMax] = new CNetSession();
        mAllContext[mMax]->setIndex(mMax);
        mAllContext[mMax]->setTime(0xFFFFFFFFFFFFFFFFULL);
        return mAllContext[mMax++];
    }
    return 0;
}

CNetSession* CNetSessionPool::getIdleSession(u64 now_ms, u32 timeinterval_ms) {
    if(mClosed) {
        return 0;
    }
    if(!mHead) {
        return createSession();
    }
    CNetSession* ret = mHead;
    if(ret->getTime() + timeinterval_ms > now_ms) {
        return createSession();
    }
    if(mHead == mTail) {
        mHead = 0;
        mTail = 0;
    } else {
        mHead = mHead->getNext();
    }
    --mIdle;
    return ret;
}

void CNetSessionPool::addIdleSession(CNetSession* it) {
    if(it) {
        it->setNext(0);
        if(mTail) {
            mTail->setNext(it);
            mTail = it;
        } else {
            mTail = it;
            mHead = it;
        }
        ++mIdle;
    }
}

void CNetSessionPool::create(u32 max) {
    APP_ASSERT(max < ENET_SESSION_MASK);
    if(0 == mMax) {
        if(max > ENET_SESSION_MASK) {
            mMax = ENET_SESSION_MASK;
        } else {
#ifdef NDEBUG
            mMax = (max < 1000 ? 1000 : max);
#else
            mMax = (max < 10 ? 10 : max);
#endif
        }
        for(u32 i = 0; i < mMax; ++i) {
            //new (&mAllContext[i]) CNetSession();
            mAllContext[i] = new CNetSession();
            mAllContext[i]->setIndex(i);
            mAllContext[i]->setTime(0xFFFFFFFFFFFFFFFFULL);
            addIdleSession(mAllContext[i]);
        }
    }
}

bool CNetSessionPool::waitClose() {
    if(mClosed) {
        return mIdle == mMax;
    }
    mClosed = true;
    for(u32 i = 0; mIdle != mMax && i < mMax; ++i) {
        mAllContext[i]->getSocket().close();
    }
    return mIdle == mMax;
}

void CNetSessionPool::clearAll() {
    for(u32 i = 0; i < mMax; ++i) {
        //mAllContext[i].~CNetSession();
        mAllContext[i]->getSocket().close();
        delete mAllContext[i];
        mAllContext[i] = 0;
    }
    //delete[] mAllContext;
    //mAllContext = 0;
    mMax = 0;
    mHead = 0;
    mTail = 0;
}


} //namespace net
} //namespace irr
