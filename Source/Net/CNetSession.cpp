#include "CNetSession.h"
#include "IAppLogger.h"
#include "CEventPoller.h"
#include "CNetServiceTCP.h"


namespace irr {
namespace net {

CNetSessionPool::CNetSessionPool() :
    mIdle(0),
    mMax(0),
    mHead(0),
    mTail(0) {
    ::memset(mAllContext, 0, sizeof(mAllContext));
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


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {


void AppTimeoutContext(void* it) {
    CNetSession& iContext = *reinterpret_cast<CNetSession*>(it);
    iContext.postTimeout();
}


CNetSession::CNetSession() :
    mService(0),
    mInGlobalQueue(0),
    mPacketSend(0),
    mPacketReceive(0),
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


bool CNetSession::disconnect(u32 id) {
    if(!isValid(id)) {
        return false;
    }
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_DISCONNECT | getIndex());
    return mService->getPoller().postEvent(evt);
}


bool CNetSession::connect(const CNetAddress& it) {
    mAddressRemote = it;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_CONNECT | getIndex());
    return mService->getPoller().postEvent(evt);
}


bool CNetSession::receive() {
    APP_ASSERT(0 == mPacketReceive);
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_RECEIVE | getIndex());
    return mService->getPoller().postEvent(evt);
}


s32 CNetSession::postSend(u32 id, CBufferQueue::SBuffer* buf) {
    if(!isValid(id) || !buf) {
        return -1;
    }
    if(ENET_CMD_SEND & mStatus) {
        CEventQueue::SNode* nd = mQueueInput.create(0);
        nd->mEvent.mType = ENET_SEND;
        nd->mEvent.mSessionID = getID();
        nd->mEventer = mEventer;
        nd->mEvent.mInfo.mDataSend.mSize = 0;
        nd->mEvent.mInfo.mDataSend.mBuffer = buf;
        mQueueInput.push(nd);
        return 0;
    }
    mStatus |= ENET_CMD_SEND;
    mPacketSend = mQueueInput.create(0);
    mPacketSend->mEventer = mEventer;
    mPacketSend->mEvent.mType = ENET_SEND;
    mPacketSend->mEvent.mSessionID = getID();
    mPacketSend->mEvent.mInfo.mDataSend.mSize = 0;
    mPacketSend->mEvent.mInfo.mDataSend.mBuffer = buf;
    mActionSend.mBuffer.buf = buf->getBuffer() + mPacketSend->mEvent.mInfo.mDataSend.mSize;
    mActionSend.mBuffer.len = buf->mBufferSize - mPacketSend->mEvent.mInfo.mDataSend.mSize;
    if(mSocket.send(&mActionSend)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    //CEventPoller::cancelIO(mSocket, 0);
    pushGlobalQueue(mPacketSend);
    mPacketSend = 0;
    return mCount;
}


s32 CNetSession::postSend() {
    if(!mPacketSend || (ENET_CMD_SEND & mStatus)) {
        return mCount;
    }
    mStatus |= ENET_CMD_SEND;
    CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
        mPacketSend->mEvent.mInfo.mDataSend.mBuffer);

    mActionSend.mBytes = 0;
    mActionSend.mBuffer.buf = buf->getBuffer() + mPacketSend->mEvent.mInfo.mDataSend.mSize;
    mActionSend.mBuffer.len = buf->mBufferSize - mPacketSend->mEvent.mInfo.mDataSend.mSize;
    if(mSocket.send(&mActionSend)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    //CEventPoller::cancelIO(mSocket, 0);
    pushGlobalQueue(mPacketSend);
    mPacketSend = 0;
    return mCount;
}


s32 CNetSession::stepSend() {
    --mCount;
    if(0 == mActionSend.mBytes) {
        //CEventPoller::cancelIO(mSocket, 0);
        APP_LOG(ELOG_WARNING, "CNetSession::stepSend",
            "quit on send: [%u] [ecode=%u]",
            mSocket.getValue(),
            CNetSocket::getError());
        mPacketSend->mEvent.mInfo.mDataSend.mSize = 0;
        pushGlobalQueue(mPacketSend);
        mPacketSend = 0;
        return mCount;
    }
    CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
        mPacketSend->mEvent.mInfo.mDataSend.mBuffer);
    mPacketSend->mEvent.mInfo.mDataSend.mSize += mActionSend.mBytes;
    if(mPacketSend->mEvent.mInfo.mDataSend.mSize == buf->mBufferSize) {
        pushGlobalQueue(mPacketSend);
        mPacketSend = mQueueInput.pop();
    }
    mStatus &= ~ENET_CMD_SEND;
    return postSend();
}


s32 CNetSession::postReceive() {
    const u32 bufsz = 1024 * 8;
    mPacketReceive = mQueueEvent.create(bufsz);
    mPacketReceive->mEvent.mType = ENET_RECEIVE;
    mPacketReceive->mEventer = mEventer;
    mPacketReceive->mEvent.mSessionID = getID();
    mPacketReceive->mEvent.mInfo.mDataReceive.mSize = 0;
    mPacketReceive->mEvent.mInfo.mDataReceive.mAllocatedSize = bufsz;
    mPacketReceive->mEvent.mInfo.mDataReceive.mBuffer
        = reinterpret_cast<c8*>(mPacketReceive) + sizeof(*mPacketReceive);
    
    //mActionReceive.init();
    mActionReceive.mBytes = 0;
    mActionReceive.mBuffer.buf = reinterpret_cast<c8*>(mPacketReceive->mEvent.mInfo.mDataReceive.mBuffer);
    mActionReceive.mBuffer.len = mPacketReceive->mEvent.mInfo.mDataReceive.mAllocatedSize;
    if(mSocket.receive(&mActionReceive)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postReceive", "ecode=%u", CNetSocket::getError());
    mPacketReceive->drop();
    mPacketReceive = 0;
    return mCount;
}


s32 CNetSession::stepReceive() {
    //APP_ASSERT(mCount > 0);
    --mCount;
    if(0 == mActionReceive.mBytes) {
        //CEventPoller::cancelIO(mSocket, 0);
        if(mPacketReceive) {
            mPacketReceive->drop();
            mPacketReceive = 0;
        }
        return mCount;
    }
    mPacketReceive->mEvent.mInfo.mDataReceive.mSize += mActionReceive.mBytes;
    pushGlobalQueue(mPacketReceive);
    mPacketReceive = 0;
    return postReceive();
}


s32 CNetSession::postConnect() {
    APP_ASSERT(1 == mCount);
    if(mSocket.connect(mAddressRemote, &mActionConnect)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postConnect", "ecode=%u", CNetSocket::getError());
    return mCount;
}


s32 CNetSession::stepConnect() {
    APP_ASSERT(mCount == 2);
    --mCount;
    if(postReceive() > 1) {
        postEvent(ENET_CONNECT);
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
    return (mSocket.disconnect(&mActionDisconnect) ? ++mCount : mCount);
}


s32 CNetSession::stepDisonnect() {
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
    evt.setMessage(ENET_CMD_TIMEOUT | getIndex());
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
        if(mPacketReceive) {
            mPacketReceive->drop();
            mPacketReceive = 0;
        }
        if(mPacketSend) {
            CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
                mPacketSend->mEvent.mInfo.mDataSend.mBuffer);
            buf->drop();
            mPacketSend->drop();
            mPacketSend = 0;
        }
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
    return --mCount;
}


s32 CNetSession::stepError() {
    return --mCount;
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
    }
    return -1;
}


void CNetSession::setSocket(const CNetSocket& it) {
    mSocket = it;
}


void CNetSession::clear() {
    mCount = 1;
    mStatus = 0;
    mEventer = 0;
    mActionConnect.init();
    mActionDisconnect.init();
    mActionSend.init();
    mActionReceive.init();
    if(mPacketSend) {
        CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
            mPacketSend->mEvent.mInfo.mDataSend.mBuffer);
        buf->drop();
        mPacketSend->drop();
        mPacketSend = 0;
    }
    if(mPacketReceive) {
        mPacketReceive->drop();
        mPacketReceive = 0;
    }

    mActionConnect.mOperationType = EOP_CONNECT;
    mActionConnect.mFlags = SContextIO::EFLAG_REUSE;

    mActionDisconnect.mOperationType = EOP_DISCONNECT;
    mActionDisconnect.mFlags = SContextIO::EFLAG_REUSE;

    mActionSend.mFlags = 0;
    mActionSend.mOperationType = EOP_SEND;

    mActionReceive.mFlags = 0;
    mActionReceive.mOperationType = EOP_RECEIVE;
}


void CNetSession::pushGlobalQueue(CEventQueue::SNode* nd) {
    APP_ASSERT(nd);
#if defined(APP_DEBUG)
    static s32 cnt = 0;
    static s32 ecnt = 0;
    AppAtomicIncrementFetch(&cnt);
    //static CMutex mtx;
    //CAutoLock ak(mtx);
#endif
    bool evt = false;

    mQueueEvent.lock();
    mQueueEvent.push(nd);
    evt = setInGlobalQueue(1);
    mQueueEvent.unlock();

    if(evt) {
#if defined(APP_DEBUG)
        AppAtomicIncrementFetch(&ecnt);
#endif
        mService->addNetEvent(*this);
    }

    APP_LOG(ELOG_INFO, "CNetSession::pushGlobalQueue", "count=%u/%u",
        AppAtomicFetchAdd(0, &ecnt), AppAtomicFetchAdd(0, &cnt));
}


void CNetSession::dispatchEvents() {
#if defined(APP_DEBUG)
    static s32 cnt = 0;
    static s32 ecnt = 0;
    AppAtomicIncrementFetch(&ecnt);
    //static CMutex mtx;
    //CAutoLock ak(mtx);
#endif

    CEventQueue que;
    mQueueEvent.lock();
    mQueueEvent.swap(que);
    mQueueEvent.unlock();
    do {
        for(CEventQueue::SNode* nd = nd = que.pop(); nd; nd = que.pop()) {
#if defined(APP_DEBUG)
            AppAtomicIncrementFetch(&cnt);
#endif
            switch(nd->mEvent.mType) {
            case ENET_RECEIVE:
            {
                nd->mEventer->onReceive(nd->mEvent.mSessionID,
                    reinterpret_cast<c8*>(nd) + sizeof(*nd),
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

    APP_ASSERT(cnt >= ecnt);
    APP_LOG(ELOG_INFO, "CNetSession::dispatchEvents", "count=%u/%u", 
        AppAtomicFetchAdd(0, &ecnt), AppAtomicFetchAdd(0, &cnt));
}

} //namespace net
} //namespace irr


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace irr {
namespace net {

void AppTimeoutContext(void* it) {
    CNetSession& iContext = *(CNetSession*) it;
    iContext.onTimeout();
}


CNetSession::CNetSession() :
    mTime(-1),
    mID(getMask(0, 1)),
    mID(getMask(0, 1)),
    //mSessionHub(0),
    mCount(0),
    mPoller(0),
    mStatus(0),
    mPacketSend(8 * 1024),
    mPacketReceive(8 * 1024),
    mEventer(0) {
    clear();
    mTimeNode.mCallback = AppTimeoutContext;
    mTimeNode.mCallbackData = this;
}


CNetSession::~CNetSession() {
}


bool CNetSession::disconnect() {
    if(mStatus & ENET_CMD_DISCONNECT) {
        return true;
    }
    mStatus |= ENET_CMD_DISCONNECT;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_DISCONNECT | getIndex());
    return mService->getPoller().postEvent(evt);
}


bool CNetSession::connect(const CNetAddress& it) {
    if(mStatus & ENET_CMD_CONNECT) {
        return true;
    }
    mStatus |= ENET_CMD_CONNECT;
    mAddressRemote = it;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_CONNECT | getIndex());
    return mService->getPoller().postEvent(evt);
}


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


bool CNetSession::onTimeout() {
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_TIMEOUT | getIndex());
    return mService->getPoller().postEvent(evt);
}


bool CNetSession::onNewSession() {
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_NEW_SESSION | getIndex());
    return mService->getPoller().postEvent(evt);
}


void CNetSession::postEvent(ENetEventType iEvent) {
    if(mEventer) {
        SNetEvent evt;
        evt.mType = iEvent;
        //evt.mInfo.mData.mBuffer = 0;
        //evt.mInfo.mData.mSize = 0;
        evt.mInfo.mSession.mSocket = &mSocket;
        evt.mInfo.mSession.mContext = this;
        evt.mInfo.mSession.mAddressLocal = &mAddressLocal;
        evt.mInfo.mSession.mAddressRemote = &mAddressRemote;
        mEventer->onEvent(evt);
    }
}


void CNetSession::setSocket(const CNetSocket& it) {
    mSocket = it;
}


void CNetSession::clear() {
    mID = mID;
    mStatus = 0;
    mCount = 0;
    mEventer = 0;
    mPacketSend.setUsed(0);
    mPacketReceive.setUsed(0);
}


} //namespace net
} //namespace irr
#endif //OS
