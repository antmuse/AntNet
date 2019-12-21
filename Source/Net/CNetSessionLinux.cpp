#include "CNetSession.h"
#include "IAppLogger.h"
#include "CEventPoller.h"
#include "CNetService.h"

#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace irr {
namespace net {

s32 CNetSession::postSend(u32 id, CBufferQueue::SBuffer* buf) {
    if(!isValid(id) || !buf) {
        return -1;
    }
    CEventQueue::SNode* nd = mQueueInput.create(sizeof(SContextIO));
    nd->mEvent.mType = ENET_SEND;
    nd->mEvent.mSessionID = getID();
    nd->mEventer = mEventer;
    nd->mEvent.mInfo.mDataSend.mSize = 0;
    nd->mEvent.mInfo.mDataSend.mBuffer = buf;
    mQueueInput.push(nd);

    /*if(ENET_CMD_SEND & mStatus) {
        return 0;
    }
    mStatus |= ENET_CMD_SEND;*/
    pushGlobalQueue(nd);
    return mCount;
}


s32 CNetSession::postSend(CEventQueue::SNode* iNode) {
    APP_LOG(ELOG_ERROR, "CNetSession::postSend", "ecode=%u", CNetSocket::getError());
    CEventQueue::SNode* nd = mQueueInput.create(0);
    nd->mEvent.mType = ENET_SEND;
    nd->mEvent.mSessionID = getID();
    nd->mEventer = mEventer;
    nd->mEvent.mInfo.mDataSend.mSize = 0;
    nd->mEvent.mInfo.mDataSend.mBuffer = nullptr;
    mQueueInput.push(nd);
    pushGlobalQueue(nd);
    return mCount;
}


s32 CNetSession::postReceive() {
    APP_LOG(ELOG_ERROR, "CNetSession::postReceive", "ecode=%u", CNetSocket::getError());
    const u32 bufsz = 1024 * 8;
    CEventQueue::SNode* nd = mQueueInput.create((u32)sizeof(SContextIO) + bufsz);
    nd->mEvent.mSessionID = getID();
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_RECEIVE;
    nd->mEvent.mInfo.mDataReceive.mSize = 0;
    nd->mEvent.mInfo.mDataReceive.mAllocatedSize = bufsz;
    nd->mEvent.mInfo.mDataReceive.mBuffer
        = reinterpret_cast<c8*>(nd + 1) + sizeof(SContextIO);

    pushGlobalQueue(nd);
    return mCount;
}


s32 CNetSession::postConnect() {
    const u32 bufsz = 1024 * 8;
    CEventQueue::SNode* nd = mQueueInput.create((u32)sizeof(SContextIO) + bufsz);
    nd->mEvent.mType = ENET_CONNECT;
    nd->mEventer = mEventer;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mDataReceive.mSize = 0;
    nd->mEvent.mInfo.mDataReceive.mAllocatedSize = bufsz;
    nd->mEvent.mInfo.mDataReceive.mBuffer = reinterpret_cast<c8*>(nd + 1) + sizeof(SContextIO);

    pushGlobalQueue(nd);
    return mCount;
}


s32 CNetSession::stepConnect(SContextIO& act) {
    //APP_ASSERT(0 == mCount);
    if(!mSocket.connect(mAddressRemote)) {
        return ++mCount;
    }
    APP_LOG(ELOG_ERROR, "CNetSession::stepConnect", "ecode=%u", CNetSocket::getError());
    return mCount;
}


s32 CNetSession::postDisconnect() {
    mSocket.close();
    //APP_LOG(ELOG_ERROR, "CNetSession::postDisconnect", "ecode=%u", CNetSocket::getError());
    return mCount;
}


s32 CNetSession::stepDisonnect(SContextIO& act) {
    return mCount;
}


void CNetSession::postTimeout() {
    CEventPoller::SEvent evt;
    evt.mData.mData64 = 0;
    evt.mEvent = (ENET_CMD_TIMEOUT | getIndex());
    mService->getPoller().postEvent(evt);
}


CEventQueue::SNode* AppCreateBuffer(CEventQueue& hub, u32 id, INetEventer* iEventer) {
    const u32 bufsz = 1024 * 8;
    CEventQueue::SNode* nd = hub.create(sizeof(SContextIO) + bufsz);
    nd->mEvent.mType = ENET_RECEIVE;
    nd->mEventer = iEventer;
    nd->mEvent.mSessionID = id;
    nd->mEvent.mInfo.mDataReceive.mSize = 0;
    nd->mEvent.mInfo.mDataReceive.mAllocatedSize = bufsz;
    nd->mEvent.mInfo.mDataReceive.mBuffer
        = reinterpret_cast<c8*>(nd + 1) + sizeof(SContextIO);
    return nd;
}


s32 CNetSession::stepReceive(SContextIO& act) {
    CEventQueue::SNode* nd = getEventNode(&act);
    nd->mEvent.mType = ENET_RECEIVE;
    s32 ecode = 0;
    while(0 == ecode) {
        const s32 max = nd->mEvent.mInfo.mDataReceive.mAllocatedSize;
        c8* buf = reinterpret_cast<c8*>(nd->mEvent.mInfo.mDataReceive.mBuffer);
        s32 recved = 0;
        for(; recved < max;) {
            s32 step = mSocket.receive(buf + recved, max - recved);
            if(step > 0) {
                recved += step;
            } else {
                ecode = mSocket.getError();
                //TODO>>
                break;
            }
        }
        if(recved > 0) {
            nd->mEvent.mInfo.mDataReceive.mSize = recved;
            nd->mEventer->onReceive(mAddressRemote, nd->mEvent.mSessionID,
                reinterpret_cast<c8*>(nd + 1) + sizeof(SContextIO),
                nd->mEvent.mInfo.mDataReceive.mSize);
        }
        nd->drop();
    }
}


s32 CNetSession::stepSend(SContextIO& act) {
    CEventQueue::SNode* nd = getEventNode(&act);
    CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
        nd->mEvent.mInfo.mDataSend.mBuffer);
    const s32 max = buf->mBufferSize;
    c8* buffer = buf->getBuffer();
    s32 snd = 0;
    for(; snd < max; ) {
        s32 step = mSocket.send(buffer + snd, max - snd);
        if(step > 0) {
            snd += step;
        } else {
            //TODO>>
            s32 ecode = mSocket.getError();
            mSocket.close();
            break;
        }
    }
    nd->mEvent.mInfo.mDataSend.mSize = snd;
    nd->mEventer->onSend(nd->mEvent.mSessionID,
        buf->getBuffer(),
        buf->mBufferSize,
        nd->mEvent.mInfo.mDataSend.mSize == buf->mBufferSize ? 0 : -1);

    buf->drop();
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
                stepReceive(*getAction(nd));
                break;
            }
            case ENET_SEND:
            {
                stepSend(*getAction(nd));
                break;
            }
            case ENET_LINKED:
            {
                nd->mEventer->onLink(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal,
                    *nd->mEvent.mInfo.mSession.mAddressRemote);
                postReceive();
                break;
            }
            case ENET_CONNECT:
            {
                if(stepConnect(*getAction(nd)) > 1) {
                    nd->mEventer->onConnect(nd->mEvent.mSessionID,
                        *nd->mEvent.mInfo.mSession.mAddressLocal,
                        *nd->mEvent.mInfo.mSession.mAddressRemote);
                    stepReceive(*getAction(nd));
                } else {
                    stepClose();
                }
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

#endif //APP_PLATFORM_LINUX or APP_PLATFORM_ANDROID
