#include "CNetSession.h"
#include "IAppLogger.h"
#include "CEventPoller.h"
#include "CNetServiceTCP.h"


namespace irr {
namespace net {
#if defined(APP_DEBUG)
static s32 G_ENQUEUE_COUNT = 0;
static s32 G_DEQUEUE_COUNT = 0;
#endif

void AppTimeoutContext(void* it) {
    CNetSession& iContext = *reinterpret_cast<CNetSession*>(it);
    iContext.postTimeout();
}
////////////////////////////////////////////////////////////////////////////////////////////////////

CNetSessionPool::CNetSessionPool() :
    mIdle(0),
    mMax(0),
    mClosed(false),
    mHead(0),
    mTail(0) {
    ::memset(mAllContext, 0, sizeof(mAllContext));
    //const s32 sz = sizeof(CNetSession);
}

CNetSessionPool::~CNetSessionPool() {
    clearAll();
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


////////////////////////////////////////////////////////////////////////////////////////////////////


CNetSession::CNetSession() :
    mService(0),
    mInGlobalQueue(0),
    mNext(0),
    mTime(-1),
    mID(getMask(0, 1)),
    mCount(1),
    mStatus(0),
    mEventer(0) {
    clear();
    mTimeNode.mCallback = AppTimeoutContext;
    mTimeNode.mCallbackData = this;
}


CNetSession::~CNetSession() {
}


void CNetSession::setService(CNetServiceTCP* it) {
    APP_ASSERT(it);
    mService = it;
    mQueueInput.setMemoryHub(&it->getMemoryHub());
    mQueueEvent.setMemoryHub(&it->getMemoryHub());
}


void CNetSession::setSocket(const CNetSocket& it) {
    mSocket = it;
}


void CNetSession::clear() {
    mCount = 1;
    mStatus = 0;
    mEventer = 0;
    mInGlobalQueue = 0;
}



void CNetSession::pushGlobalQueue(CEventQueue::SNode* nd) {
    APP_ASSERT(nd);
    mQueueEvent.lock();
    mQueueEvent.push(nd);
    bool evt = setInGlobalQueue(1);
    mQueueEvent.unlock();

    if(evt) {
#if defined(APP_DEBUG)
        AppAtomicIncrementFetch(&G_ENQUEUE_COUNT);
#endif
        mService->addNetEvent(*this);
    }
}



s32 CNetSession::postEvent(ENetEventType iEvent) {
    if(mEventer) {
        CEventQueue::SNode* nd = mQueueEvent.create(0);
        nd->mEventer = mEventer;
        nd->mEvent.mType = iEvent;
        nd->mEvent.mSessionID = getID();
        nd->mEvent.mInfo.mSession.mAddressLocal = &mAddressLocal;
        nd->mEvent.mInfo.mSession.mAddressRemote = &mAddressRemote;
        pushGlobalQueue(nd);
        if(ENET_DISCONNECT == iEvent) {
            upgradeLevel();
            mEventer = 0;
        }
        return 0;
    }
    return -1;
}


} //namespace net
} //namespace irr


////////////////////////////////////////////////////////////////////////////////////////////////////


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {

s32 CNetSession::postSend(u32 id, CBufferQueue::SBuffer* buf) {
    if(!isValid(id) || !buf) {
        return -1;
    }
    if(ENET_CMD_SEND & mStatus) {
        CEventQueue::SNode* nd = mQueueInput.create(sizeof(SContextIO));
        nd->mEvent.mType = ENET_SEND;
        nd->mEvent.mSessionID = getID();
        nd->mEventer = mEventer;
        nd->mEvent.mInfo.mDataSend.mSize = 0;
        nd->mEvent.mInfo.mDataSend.mBuffer = buf;
        mQueueInput.push(nd);
        return 0;
    }
    mStatus |= ENET_CMD_SEND;
    CEventQueue::SNode* nd = mQueueInput.create(sizeof(SContextIO));
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_SEND;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mDataSend.mSize = 0;
    nd->mEvent.mInfo.mDataSend.mBuffer = buf;
    SContextIO* act = getAction(nd);
    act->clear();
    act->mOperationType = EOP_SEND;
    act->mBuffer.buf = buf->getBuffer() + nd->mEvent.mInfo.mDataSend.mSize;
    act->mBuffer.len = buf->mBufferSize - nd->mEvent.mInfo.mDataSend.mSize;
    if(mSocket.send(act)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    //CEventPoller::cancelIO(mSocket, 0);
    pushGlobalQueue(nd);
    return mCount;
}


s32 CNetSession::postSend(CEventQueue::SNode* nd) {
    if(0 == nd || (ENET_CMD_SEND & mStatus)) {
        return mCount;
    }
    mStatus |= ENET_CMD_SEND;
    CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
        nd->mEvent.mInfo.mDataSend.mBuffer);

    SContextIO* act = getAction(nd);
    act->clear();
    act->mOperationType = EOP_SEND;
    act->mBuffer.buf = buf->getBuffer() + nd->mEvent.mInfo.mDataSend.mSize;
    act->mBuffer.len = buf->mBufferSize - nd->mEvent.mInfo.mDataSend.mSize;
    if(mSocket.send(act)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    //CEventPoller::cancelIO(mSocket, 0);
    pushGlobalQueue(nd);
    return mCount;
}


s32 CNetSession::stepSend(SContextIO& act) {
    --mCount;
    CEventQueue::SNode* nd = getEventNode(&act);
    if(0 == act.mBytes) {
        //CEventPoller::cancelIO(mSocket, 0);
        APP_LOG(ELOG_WARNING, "CNetSession::stepSend",
            "quit on send: [%u] [ecode=%u]",
            mSocket.getValue(),
            CNetSocket::getError());
        nd->mEvent.mInfo.mDataSend.mSize = 0;
        pushGlobalQueue(nd);
        return mCount;
    }
    CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
        nd->mEvent.mInfo.mDataSend.mBuffer);
    nd->mEvent.mInfo.mDataSend.mSize += act.mBytes;
    if(nd->mEvent.mInfo.mDataSend.mSize == buf->mBufferSize) {
        pushGlobalQueue(nd);
        nd = mQueueInput.pop();
    }
    mStatus &= ~ENET_CMD_SEND;
    return postSend(nd);
}


s32 CNetSession::postReceive() {
    const u32 bufsz = 1024 * 8;
    CEventQueue::SNode* nd = mQueueEvent.create(sizeof(SContextIO) + bufsz);
    nd->mEvent.mType = ENET_RECEIVE;
    nd->mEventer = mEventer;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mDataReceive.mSize = 0;
    nd->mEvent.mInfo.mDataReceive.mAllocatedSize = bufsz;
    nd->mEvent.mInfo.mDataReceive.mBuffer
        = reinterpret_cast<c8*>(nd + 1) + sizeof(SContextIO);

    SContextIO* act = getAction(nd);
    act->clear();
    act->mOperationType = EOP_RECEIVE;
    act->mBuffer.buf = reinterpret_cast<c8*>(nd->mEvent.mInfo.mDataReceive.mBuffer);
    act->mBuffer.len = nd->mEvent.mInfo.mDataReceive.mAllocatedSize;
    if(mSocket.receive(act)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postReceive", "ecode=%u", CNetSocket::getError());
    nd->drop();
    return mCount;
}


s32 CNetSession::stepReceive(SContextIO& act) {
    //APP_ASSERT(&act == mPacketReceive);
    --mCount;
    CEventQueue::SNode* nd = getEventNode(&act);
    if(0 == act.mBytes) {
        //CEventPoller::cancelIO(mSocket, 0);
        nd->drop();
        return mCount;
    }
    nd->mEvent.mInfo.mDataReceive.mSize += act.mBytes;
    pushGlobalQueue(nd);
    return postReceive();
}


s32 CNetSession::postConnect() {
    APP_ASSERT(1 == mCount);
    CEventQueue::SNode* nd = mQueueEvent.create(sizeof(SContextIO));
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_CONNECT;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mSession.mAddressLocal = &mAddressLocal;
    nd->mEvent.mInfo.mSession.mAddressRemote = &mAddressRemote;

    SContextIO* act = getAction(nd);
    act->clear();
    act->mOperationType = EOP_CONNECT;
    act->mFlags = SContextIO::EFLAG_REUSE;
    if(mSocket.connect(mAddressRemote, act)) {
        ++mCount;
    } else {
        nd->drop();
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postConnect", "ecode=%u", CNetSocket::getError());
    return mCount;
}


s32 CNetSession::stepConnect(SContextIO& act) {
    APP_ASSERT(mCount == 2);
    CEventQueue::SNode* nd = getEventNode(&act);
    --mCount;
    if(postReceive() > 1) {
        pushGlobalQueue(nd);
    } else {
        nd->drop();
    }
    //APP_LOG(ELOG_ERROR, "CNetSession::stepConnect", "ecode=%u", CNetSocket::getError());
    return mCount;
}


s32 CNetSession::postDisconnect() {
    if(ENET_CMD_DISCONNECT & mStatus) {
        return mCount;
    }
    mStatus |= ENET_CMD_DISCONNECT;
    //APP_LOG(ELOG_ERROR, "CNetSession::postDisconnect", "ecode=%u", CNetSocket::getError());

    CEventQueue::SNode* nd = mQueueEvent.create(sizeof(SContextIO));
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_DISCONNECT;
    nd->mEvent.mSessionID = getID();

    SContextIO* act = getAction(nd);
    act->clear();
    act->mOperationType = EOP_DISCONNECT;
    act->mFlags = SContextIO::EFLAG_REUSE;
    if(mSocket.disconnect(act)) {
        ++mCount;
    } else {
        nd->drop();
    }
    return mCount;
}


s32 CNetSession::stepDisonnect(SContextIO& act) {
    CEventQueue::SNode* nd = getEventNode(&act);
    nd->drop();
    return --mCount;
}


void CNetSession::postTimeout() {
    APP_LOG(ELOG_INFO, "CNetSession::postTimeout",
        "%u/%u,remote[%s:%u]",
        mCount,
        mID,
        mAddressRemote.getIPString(),
        mAddressRemote.getPort());

    CEventPoller::SEvent evt;
    evt.mPointer = 0;
    evt.mKey = (ENET_CMD_TIMEOUT | getIndex());
    mService->getPoller().postEvent(evt);
}


s32 CNetSession::stepClose() {
    APP_ASSERT(0 == mCount);
    APP_LOG(ELOG_INFO, "CNetSession::stepClose",
        "%u/%u,remote[%s:%u]",
        mCount,
        mID,
        mAddressRemote.getIPString(),
        mAddressRemote.getPort());
    if(0 == mCount) {
        postEvent(ENET_DISCONNECT);
        mSocket.close();
    }
    return mCount;
}


s32 CNetSession::stepTimeout() {
    APP_ASSERT(mCount >= 0);
    APP_LOG(ELOG_INFO, "CNetSession::stepTimeout",
        "%u/%u,remote[%s:%u]",
        mCount,
        mID,
        mAddressRemote.getIPString(),
        mAddressRemote.getPort());

    if(1 == mCount) {
        CEventQueue que;
        mQueueInput.lock();
        mQueueInput.swap(que);
        mQueueInput.unlock();

        bool notept = !que.isEmpty();
        mQueueEvent.lock();
        if(notept) {
            mQueueEvent.push(que);
        } else {
            notept = !mQueueEvent.isEmpty();
        }
        bool evt = (notept && setInGlobalQueue(1));
        mQueueEvent.unlock();
        if(evt) {
            mService->addNetEvent(*this);
#if defined(APP_DEBUG)
            AppAtomicIncrementFetch(&G_ENQUEUE_COUNT);
#endif
        }
        APP_LOG(ELOG_INFO, "CNetSession::stepTimeout",
            "enqueue=%u, dequeue=%u",
            G_ENQUEUE_COUNT, G_DEQUEUE_COUNT);
        if(notept) {
            return mCount;
        }
    }
    return --mCount;
}


void CNetSession::dispatchEvents() {
#if defined(APP_DEBUG)
    AppAtomicIncrementFetch(&G_DEQUEUE_COUNT);
#endif

    CEventQueue que;
    mQueueEvent.lock();
    mQueueEvent.swap(que);
    mQueueEvent.unlock();
    do {
        for(CEventQueue::SNode* nd = nd = que.pop(); nd; nd = que.pop()) {
            switch(nd->mEvent.mType) {
            case ENET_RECEIVE:
            {
                nd->mEventer->onReceive(nd->mEvent.mSessionID,
                    reinterpret_cast<c8*>(nd + 1) + sizeof(SContextIO),
                    nd->mEvent.mInfo.mDataReceive.mSize);
                break;
            }
            case ENET_SEND:
            {
                CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
                    nd->mEvent.mInfo.mDataSend.mBuffer);
                nd->mEventer->onSend(nd->mEvent.mSessionID,
                    buf->getBuffer(),
                    buf->mBufferSize,
                    nd->mEvent.mInfo.mDataSend.mSize == buf->mBufferSize ? 0 : -1);
                buf->drop();
                break;
            }
            case ENET_LINKED:
            {
                nd->mEventer->onLink(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal,
                    *nd->mEvent.mInfo.mSession.mAddressRemote);
                break;
            }
            case ENET_CONNECT:
            {
                nd->mEventer->onConnect(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal,
                    *nd->mEvent.mInfo.mSession.mAddressRemote);
                break;
            }
            case ENET_DISCONNECT:
            {
                nd->mEventer->onDisconnect(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal,
                    *nd->mEvent.mInfo.mSession.mAddressRemote);
                break;
            }
            case ENET_TIMEOUT:
            {
                //TODO>>
                break;
            }
            default:
                APP_ASSERT(0);
                break;
            }//switch
            nd->drop();
        }//for

        mQueueEvent.lock();
        mQueueEvent.swap(que);
        if(que.isEmpty()) {
            popGlobalQueue();
        }
        mQueueEvent.unlock();
    } while(!que.isEmpty());
}

} //namespace net
} //namespace irr


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace irr {
namespace net {

s32 CNetSession::send(const void* iBuffer, s32 iSize) {
    if(!iBuffer || iSize < 0) {//0 byte is ok
        return -1;
    }
    if(ENET_CMD_SEND & mStatus) {
        return 0;
    }
    mStatus |= ENET_CMD_SEND;
    iSize = (iSize > mPacketSend.getWriteSize() ? mPacketSend.getWriteSize() : iSize);
    mPacketSend.addBuffer(iBuffer, iSize);
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_SEND | getIndex());
    return mService->getPoller().postEvent(evt) ? iSize : -1;
}


s32 CNetSession::postSend() {
    if(!isValid()) {
        return -1;
    }
    c8* buffer = mPacketSend.getPointer();
    u32 sz = mPacketSend.getSize();
    if(mSocket.send(buffer, sz)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    return -1;
}


s32 CNetSession::stepSend() {
    //CAutoLock alock(mMutex);
    --mCount;
    if(mPacketSend.clear(u32(0)) > 0) {
        return postSend();
    }

    mStatus &= ~ENET_CMD_SEND;
    postEvent(ENET_SENT);
    return mCount;
}


s32 CNetSession::postReceive() {
    //CAutoLock alock(mMutex);
    APP_ASSERT(mPacketReceive.getWriteSize() > 0);
    c8* buffer = mPacketReceive.getWritePointer();
    u32 sz = mPacketReceive.getWriteSize();
    if(mSocket.receive(buffer, sz)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postReceive", "ecode=%u", CNetSocket::getError());
    return -1;
}


s32 CNetSession::stepReceive() {
    //CAutoLock alock(mMutex);
    --mCount;
    return 0;
}


s32 CNetSession::postConnect() {
    //APP_ASSERT(0 == mCount);
    if(!isValid()) {
        return -1;
    }
    if(!mSocket.connect(mAddressRemote)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postConnect", "ecode=%u", CNetSocket::getError());
    return -1;
}


s32 CNetSession::stepConnect() {
    --mCount;
    if(postReceive() > 0) {
        postEvent(ENET_CONNECTED);
    }
    //APP_LOG(ELOG_ERROR, "CNetSession::stepConnect", "ecode=%u", CNetSocket::getError());
    return mCount;
}


s32 CNetSession::postDisconnect() {
    //重复发起postDisconnect事件则返回-1，据此强制关闭socket
    if(!isValid()) {
        return -1;
    }
    upgradeLevel();//make this context invalid
    if(mSocket.close()) {
        return ++mCount;
    }
    //还未连接成功
    //postEvent(ENET_CONNECT_TIMEOUT);
    APP_LOG(ELOG_ERROR, "CNetSession::postDisconnect", "ecode=%u", CNetSocket::getError());
    return -1;
}


s32 CNetSession::stepDisonnect() {
    --mCount;
    if(!mSocket.isOpen()) {
        return -1;
    }
    postEvent(ENET_DISCONNECTED); //step1
    mEventer = 0;           //step2
    return mCount;
}


void CNetSession::postTimeout() {
    CEventPoller::SEvent evt;
    evt.mEvent = (ENET_CMD_TIMEOUT | getIndex());
    return mService->getPoller().postEvent(evt);
}


} //namespace net
} //namespace irr
#endif //OS
