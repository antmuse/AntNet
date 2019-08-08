#include "CNetSession.h"
#include "IAppLogger.h"
#include "CEventPoller.h"
#include "CNetService.h"


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
        APP_LOG(ELOG_INFO, "CNetSession::stepReceive",
            "ID=%u,mCount=%u,remote[%s:%u]",
            mID,
            mCount,
            mAddressRemote.getIPString(),
            mAddressRemote.getPort());
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
        APP_LOG(ELOG_ERROR, "CNetSession::postConnect", "ecode=%u", CNetSocket::getError());
        nd->drop();
    }
    return mCount;
}


s32 CNetSession::stepConnect(SContextIO& act) {
    APP_ASSERT(mCount == 2);
    CEventQueue::SNode* nd = getEventNode(&act);
    --mCount;
    if(postReceive() > 1) {
        pushGlobalQueue(nd);
    } else {
        APP_LOG(ELOG_ERROR, "CNetSession::stepConnect", "ecode=%u", CNetSocket::getError());
        nd->drop();
    }
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

    if(mCount > 1) {
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
        } else {
            CEventQueue::SNode* nd = mQueueEvent.create(0);
            nd->mEventer = mEventer;
            nd->mEvent.mType = ENET_TIMEOUT;
            nd->mEvent.mSessionID = getID();
            nd->mEvent.mInfo.mSession.mAddressLocal = &mAddressLocal;
            nd->mEvent.mInfo.mSession.mAddressRemote = &mAddressRemote;

            mQueueEvent.lock();
            mQueueEvent.push(nd);
            bool evt = setInGlobalQueue(1);
            mQueueEvent.unlock();
            if(evt) {
                mService->addNetEvent(*this);
#if defined(APP_DEBUG)
                AppAtomicIncrementFetch(&G_ENQUEUE_COUNT);
#endif
            }
            return mCount;
        }
    }//if

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
                nd->mEventer->onTimeout(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal,
                    *nd->mEvent.mInfo.mSession.mAddressRemote);
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

#endif //APP_PLATFORM_WINDOWS
