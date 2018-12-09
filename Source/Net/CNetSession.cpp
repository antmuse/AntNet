#include "CNetSession.h"
#include "CEventPoller.h"
#include "CNetClientSeniorTCP.h"
#include "IAppLogger.h"


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
    CNetSession& iContext = *(CNetSession*) it;
    iContext.onTimeout();
}


CNetSession::CNetSession() :
    mPostedDisconnect(false),
    mNext(0),
    mTime(-1),
    mMask(getMask(0, 1)),
    //mSessionHub(0),
    //mFunctionConnect(0),
    //mFunctionDisonnect(0),
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


bool CNetSession::disconnect(u32 id) {
    APP_ASSERT(mPoller);
    if(!isValid(id)) {
        return false;
    }
    if(mStatus & ENET_CMD_DISCONNECT) {
        return true;
    }
    mStatus |= ENET_CMD_DISCONNECT;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_DISCONNECT | getIndex());
    return mPoller->postEvent(evt);
}


bool CNetSession::connect(u32 id, const CNetAddress& it) {
    APP_ASSERT(mPoller);
    if(!isValid(id)) {
        return false;
    }
    if(mStatus & ENET_CMD_CONNECT) {
        return true;
    }
    mStatus |= ENET_CMD_CONNECT;
    mAddressRemote = it;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_CONNECT | getIndex());
    return mPoller->postEvent(evt);
}


s32 CNetSession::send(u32 id, const void* iBuffer, s32 iSize) {
    //APP_ASSERT(mPoller);
    if(!isValid(id)) {
        return -1;
    }
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
    return mPoller->postEvent(evt) ? iSize : -1;
}


s32 CNetSession::postSend() {
    mActionSend.mBuffer.buf = mPacketSend.getPointer();
    mActionSend.mBuffer.len = mPacketSend.getSize();
    if(mSocket.send(&mActionSend)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    return -1;
}


s32 CNetSession::stepSend() {
    //CAutoLock alock(mMutex);
    --mCount;
    if(0 == mActionSend.mBytes) {
        APP_LOG(ELOG_WARNING, "CNetSession::stepSend",
            "quit on send: [%u] [ecode=%u]",
            mSocket.getValue(),
            CNetSocket::getError());

        return postDisconnect();
    }


    if(mPacketSend.clear(mActionSend.mBytes) > 0) {
        return postSend();
    }

    mStatus &= ~ENET_CMD_SEND;
    postEvent(ENET_SENT);
    return mCount;
}


s32 CNetSession::postReceive() {
    //CAutoLock alock(mMutex);
    APP_ASSERT(mPacketReceive.getWriteSize() > 0);
    mActionReceive.mBuffer.buf = mPacketReceive.getWritePointer();
    mActionReceive.mBuffer.len = mPacketReceive.getWriteSize();
    if(mSocket.receive(&mActionReceive)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::postReceive", "ecode=%u", CNetSocket::getError());
    return -1;
}


s32 CNetSession::stepReceive() {
    //CAutoLock alock(mMutex);
    --mCount;
    if(0 == mActionReceive.mBytes) {
        return postDisconnect();
    }

    mPacketReceive.setUsed(mPacketReceive.getSize() + mActionReceive.mBytes);

    if(mEventer) {
        SNetEvent evt;
        evt.mType = ENET_RECEIVED;
        evt.mInfo.mData.mContext = getID();
        evt.mInfo.mData.mBuffer = mPacketReceive.getReadPointer();
        evt.mInfo.mData.mSize = mPacketReceive.getReadSize();
        s32 consumed = mEventer->onEvent(evt);
        if(consumed > 0) {
            mPacketReceive.clear(consumed);
        } else if(0 == consumed) {
            if(0 == mPacketReceive.getWriteSize()) {
                mPacketReceive.setUsed(0);
            }
        } else {
            mPacketReceive.setUsed(0);
        }
    } else {
        mPacketReceive.setUsed(0);
    }
    return postReceive();
}


s32 CNetSession::postConnect() {
    //APP_ASSERT(0 == mCount);
    if(mSocket.connect(mAddressRemote, &mActionConnect)) {
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
    //重复发起postDisconnect事件则返回-1，据此强制关闭socket, TODO>>
    if(mPostedDisconnect) {
        return -1;
    }
    mPostedDisconnect = true;
    upgradeLevel();
    if(mSocket.disconnect(&mActionDisconnect)) {
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
    postEvent(ENET_CLOSED); //step1
    mEventer = 0;           //step2
    return mCount;
}


bool CNetSession::onTimeout() {
    APP_ASSERT(mPoller);
    IAppLogger::log(ELOG_INFO, "CNetSession::onTimeout",
        "remote[%s:%u]", mAddressRemote.getIPString(), mAddressRemote.getPort());
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_TIMEOUT | getIndex());
    return mPoller->postEvent(evt);
}


bool CNetSession::onNewSession() {
    APP_ASSERT(mPoller);
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_NEW_SESSION | getIndex());
    return mPoller->postEvent(evt);
}


void CNetSession::postEvent(ENetEventType iEvent) {
    if(mEventer) {
        SNetEvent evt;
        evt.mType = iEvent;
        //evt.mInfo.mData.mBuffer = 0;
        //evt.mInfo.mData.mSize = 0;
        evt.mInfo.mSession.mSocket = &mSocket;
        evt.mInfo.mSession.mContext = getID();
        evt.mInfo.mSession.mAddressLocal = &mAddressLocal;
        evt.mInfo.mSession.mAddressRemote = &mAddressRemote;
        mEventer->onEvent(evt);
    }
}


void CNetSession::setSocket(const CNetSocket& it) {
    mSocket = it;
    //mFunctionConnect = mSocket.getFunctionConnect();
    //mFunctionDisonnect = mSocket.getFunctionDisconnect();
}


void CNetSession::clear() {
    mPostedDisconnect = false;
    mStatus = 0;
    mCount = 0;
    mEventer = 0;
    mActionConnect.init();
    mActionDisconnect.init();
    mActionSend.init();
    mActionReceive.init();
    mPacketSend.setUsed(0);
    mPacketReceive.setUsed(0);

    mActionConnect.mOperationType = EOP_CONNECT;
    mActionConnect.mFlags = SContextIO::EFLAG_REUSE;

    mActionDisconnect.mOperationType = EOP_DISCONNECT;
    mActionDisconnect.mFlags = SContextIO::EFLAG_REUSE;

    mActionSend.mFlags = 0;
    mActionSend.mOperationType = EOP_SEND;

    mActionReceive.mFlags = 0;
    mActionReceive.mOperationType = EOP_RECEIVE;
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
    mMask(getMask(0, 1)),
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
    APP_ASSERT(mPoller);
    if(mStatus & ENET_CMD_DISCONNECT) {
        return true;
    }
    mStatus |= ENET_CMD_DISCONNECT;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_DISCONNECT | getIndex());
    return mPoller->postEvent(evt);
}


bool CNetSession::connect(const CNetAddress& it) {
    APP_ASSERT(mPoller);
    if(mStatus & ENET_CMD_CONNECT) {
        return true;
    }
    mStatus |= ENET_CMD_CONNECT;
    mAddressRemote = it;
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_CONNECT | getIndex());
    return mPoller->postEvent(evt);
}


s32 CNetSession::send(const void* iBuffer, s32 iSize) {
    //APP_ASSERT(mPoller);
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
    return mPoller->postEvent(evt) ? iSize : -1;
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
    postEvent(ENET_CLOSED); //step1
    mEventer = 0;           //step2
    return mCount;
}


bool CNetSession::onTimeout() {
    APP_ASSERT(mPoller);
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_TIMEOUT | getIndex());
    return mPoller->postEvent(evt);
}


bool CNetSession::onNewSession() {
    APP_ASSERT(mPoller);
    CEventPoller::SEvent evt;
    evt.setMessage(ENET_CMD_NEW_SESSION | getIndex());
    return mPoller->postEvent(evt);
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
    mID = mMask;
    mStatus = 0;
    mCount = 0;
    mEventer = 0;
    mPacketSend.setUsed(0);
    mPacketReceive.setUsed(0);
}


} //namespace net
} //namespace irr
#endif //OS
