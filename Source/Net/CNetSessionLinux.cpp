#include "CNetSession.h"
#include "CLogger.h"
#include "CEventPoller.h"
#include "CNetService.h"

#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace app {
namespace net {

s32 CNetSession::postSend(u32 id, CBufferQueue::SBuffer* buf) {
    if(!isValid(id) || !buf) {
        return -1;
    }
    CEventQueue::SNode* nd = mQueueInput.create(0);
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_SEND;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mDataSend.mSize = 0;
    nd->mEvent.mInfo.mDataSend.mBuffer = buf;
    mQueueInput.push(nd);
    return postSend();
}


s32 CNetSession::postSend() {
    //if(ENET_CMD_SEND & mStatus) {
    //    return mCount;
    //}
    //mStatus |= ENET_CMD_SEND;
    stepConnect();

    CEventQueue::SNode* nd = mQueueEvent.create(0);
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_SEND;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mDataSend.mSize = 0;
    nd->mEvent.mInfo.mDataSend.mBuffer = nullptr;

    pushGlobalQueue(nd);
    return ++mCount;
}


s32 CNetSession::postReceive() {
    //APP_LOG(ELOG_ERROR, "CNetSession::postReceive", "ecode=%u", CNetSocket::getError());
    const u32 bufsz = 1024 * 8;
    CEventQueue::SNode* nd = mQueueInput.create((u32)sizeof(SContextIO) + bufsz);
    nd->mEvent.mSessionID = getID();
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_RECEIVE;
    nd->mEvent.mInfo.mDataReceive.mSize = 0;
    nd->mEvent.mInfo.mDataReceive.mAllocatedSize = bufsz;
    nd->mEvent.mInfo.mDataReceive.mBuffer = reinterpret_cast<s8*>(nd + 1) + sizeof(SContextIO);

    pushGlobalQueue(nd);
    return ++mCount;
}


s32 CNetSession::postConnect() {
    if(0 == mSocket.connect(mAddressRemote)) {
        stepConnect();
        return ++mCount;
    }
    const s32 ecode = CNetSocket::getError();
    if(EINPROGRESS == ecode) {
        return ++mCount;
    }
    CLogger::log(ELOG_ERROR, "CNetSession::postConnect", "ecode=%u", ecode);
    return -1;
}


s32 CNetSession::stepConnect() {
    if(ENET_CMD_CONNECT & mStatus) {
        return mCount;
    }
    mStatus |= ENET_CMD_CONNECT;

    //APP_ASSERT(1 == mCount);
    CEventQueue::SNode* nd = mQueueEvent.create(0);
    nd->mEventer = mEventer;
    nd->mEvent.mType = ENET_CONNECT;
    nd->mEvent.mSessionID = getID();
    nd->mEvent.mInfo.mSession.mAddressLocal = &mAddressLocal;
    nd->mEvent.mInfo.mSession.mAddressRemote = &mAddressRemote;

    pushGlobalQueue(nd);
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


s32 CNetSession::stepReceive(SContextIO& act) {
    CEventQueue::SNode* nd = getEventNode(&act);
    nd->mEvent.mType = ENET_RECEIVE;
    const s32 max = nd->mEvent.mInfo.mDataReceive.mAllocatedSize;
    s8* buf = reinterpret_cast<s8*>(nd->mEvent.mInfo.mDataReceive.mBuffer);
    while(true) {
        s32 step = mSocket.receive(buf, max);
        if(step > 0) {
            nd->mEvent.mInfo.mDataReceive.mSize = step;
            nd->mEventer->onReceive(mAddressRemote, nd->mEvent.mSessionID, buf, step);
        } else if(0 == step) {
            CLogger::log(ELOG_INFO, "CNetSession::stepReceive", "recv=0, sock=%d", mSocket.getValue());
            break;
        } else {
            s32 ecode = mSocket.getError();
            if(EAGAIN != ecode) {
                CLogger::log(ELOG_ERROR, "CNetSession::stepReceive", "recv<0, ecode=%d", ecode);
            }
            break;
        }
    }//while
    nd->drop();
    return --mCount;
}


s32 CNetSession::stepSend() {
    //mStatus &= ~ENET_CMD_SEND;
    --mCount;
    CEventQueue::SNode* nd = mQueueInput.lockPick();
    if(nullptr == nd) {
        CLogger::log(ELOG_ERROR, "CNetSession::stepSend", "none buf");
        return mCount;
    }
    bool post = true;
    CBufferQueue::SBuffer* buf = reinterpret_cast<CBufferQueue::SBuffer*>(
        nd->mEvent.mInfo.mDataSend.mBuffer);
    const s32 max = buf->mBufferSize;
    s8* buffer = buf->getBuffer();
    s32 snd = nd->mEvent.mInfo.mDataSend.mSize;
    for(; snd < max; ) {
        s32 step = mSocket.send(buffer + snd, max - snd);
        if(step > 0) {
            snd += step;
        } else if(0 == step) {
            CLogger::log(ELOG_INFO, "CNetSession::stepSend", "send=0, sock=%d", mSocket.getValue());
            break;
        } else {
            s32 ecode = mSocket.getError();
            if(EAGAIN != ecode) {
                CLogger::log(ELOG_ERROR, "CNetSession::stepSend", "send<0, ecode=%d", ecode);
            } else {
                post = false;
            }
            break;
        }
    }

    nd->mEvent.mInfo.mDataSend.mSize = snd;
    if(snd == max) {
        nd->mEventer->onSend(nd->mEvent.mSessionID,
            buf->getBuffer(),
            buf->mBufferSize,
            nd->mEvent.mInfo.mDataSend.mSize == buf->mBufferSize ? 0 : -1);
        mQueueInput.lockPop();
        buf->drop();
        nd->drop();
        //post = mQueueInput.lockPick() != nullptr;
    }
    if(post) {
        CEventQueue::SNode* nd = mQueueEvent.create(0);
        nd->mEventer = mEventer;
        nd->mEvent.mType = ENET_SEND;
        nd->mEvent.mSessionID = getID();
        nd->mEvent.mInfo.mDataSend.mSize = 0;
        nd->mEvent.mInfo.mDataSend.mBuffer = nullptr;

        pushGlobalQueue(nd);
        return ++mCount;
    }
    return mCount;
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
    //APP_ASSERT(mCount >= 0);
    APP_LOG(ELOG_INFO, "CNetSession::stepTimeout",
        "%u/%u,remote[%s:%u]",
        mCount,
        mID,
        mAddressRemote.getIPString(),
        mAddressRemote.getPort());
    return 1; //TODO


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
                stepSend();
                break;
            }
            case ENET_LINKED:
            {
                mStatus |= ENET_CMD_CONNECT;
                nd->mEventer->onLink(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal, *nd->mEvent.mInfo.mSession.mAddressRemote);
                break;
            }
            case ENET_CONNECT:
            {
                nd->mEventer->onConnect(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal, *nd->mEvent.mInfo.mSession.mAddressRemote);
                break;
            }
            case ENET_DISCONNECT:
            {
                nd->mEventer->onDisconnect(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal, *nd->mEvent.mInfo.mSession.mAddressRemote);
                break;
            }
            case ENET_TIMEOUT:
            {
                nd->mEventer->onTimeout(nd->mEvent.mSessionID,
                    *nd->mEvent.mInfo.mSession.mAddressLocal, *nd->mEvent.mInfo.mSession.mAddressRemote);
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
} //namespace app

#endif //APP_PLATFORM_LINUX or APP_PLATFORM_ANDROID
