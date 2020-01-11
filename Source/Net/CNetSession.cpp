#include "CNetSession.h"
#include "IAppLogger.h"
#include "CEventPoller.h"
#include "CNetService.h"


namespace irr {
namespace net {

void AppTimeoutContext(void* it) {
    CNetSession& iContext = *reinterpret_cast<CNetSession*>(it);
    iContext.postTimeout();
}

#if defined(APP_DEBUG)
s32 CNetSession::G_ENQUEUE_COUNT = 0;
s32 CNetSession::G_DEQUEUE_COUNT = 0;
#endif

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
    mCount = 1; //1 for timeout
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
