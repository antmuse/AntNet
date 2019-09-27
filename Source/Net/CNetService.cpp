#include "CNetService.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "CMemoryHub.h"
#include "CNetUtility.h"

#define APP_NET_SESSION_TIMEOUT     5000
#define APP_NET_SESSION_LINGER      8000


namespace irr {
namespace net {

CNetServiceTCP::CNetServiceTCP() :
    mID(0),
    mConfig(nullptr),
    mThread(0),
    mThreadPool(0),
    mMemHub(0),
    mCreatedSocket(0),
    mClosedSocket(0),
    mTotalReceived(0),
    mWheel(0, 200),//APP_NET_SESSION_TIMEOUT/5
    mRunning(false),
    mReceiver(0),
    mStartTime(0),
    mCurrentTime(0),
    mTimeInterval(1000) {
    CNetUtility::loadSocketLib();
}


CNetServiceTCP::~CNetServiceTCP() {
    stop();
    CNetUtility::unloadSocketLib();
    if(mConfig) {
        mConfig->drop();
        mConfig = 0;
    }
}

CNetConfig* CNetServiceTCP::setConfig(CNetConfig* cfg) {
    if(cfg) {
        if(mConfig) {
            mConfig->drop();
        }
        cfg->grab();
        mConfig = cfg;
        mTimeInterval = cfg->mPollTimeout;
    }
    return mConfig;
}

bool CNetServiceTCP::start(CNetConfig* cfg) {
    if(mRunning) {
        return true;
    }
    if(nullptr == setConfig(cfg)) {
        return false;
    }
#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    CNetSocket& sock = mPoller.getSocketPair().getSocketA();
    if(!sock.isOpen()) {
        //printf("error %d on socketpair\n", errno);
        return false;
    }
    CEventPoller::SEvent evt;
    evt.mData.mData32 = ENET_SESSION_MASK;
    evt.mEvent = EPOLLIN | EPOLLERR;
    if(!mPoller.add(sock, evt)) {
        return false;
    }
#endif //LINUX, ANDROID

    if(!mMemHub) {
        CMemoryHub* hub = new CMemoryHub();
        setMemoryHub(hub);
        hub->drop();
    }
    mRunning = true;
    mThreadPool = new CThreadPool(mConfig->mMaxWorkThread);
    mThreadPool->start();
    mTotalReceived = 0;
    mClosedSocket = 0;
    mCurrentTime = IAppTimer::getTime();
    mStartTime = mCurrentTime;
    mSessionPool.create(mConfig->mMaxContext);
    mThread = new CThread();
    mThread->start(*this);
    return true;
}


bool CNetServiceTCP::stop() {
    if(!mRunning) {
        //IAppLogger::log(ELOG_INFO, "CNetServiceTCP::stop", "server had stoped.");
        return true;
    }
    for(bool clean = false; !clean; mThread->sleep(100)) {
        mMutex.lock();
        clean = mSessionPool.waitClose();
        mMutex.unlock();
    }
    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::stop", "all session exited");
    mRunning = false;
    mCurrentTime = IAppTimer::getTime();
    if(activePoller(ENET_SESSION_MASK)) {
        mThread->join();
        delete mThread;
        mThread = 0;
        mThreadPool->join();
        delete mThreadPool;
        mThreadPool = 0;
        //APP_LOG(ELOG_INFO, "CNetServiceTCP::stop", "server stoped success");
        //mQueueSend.clear();
        clearBuffers();
        if(mMemHub) {
            mMemHub->drop();
        }
        IAppLogger::log(ELOG_INFO, "CNetServiceTCP::stop",
            "Statistics: [session=%u][speed=%lluKb/s][size=%lluKb][seconds=%llu]",
            mCreatedSocket,
            (mTotalReceived >> 10) / ((mCurrentTime - mStartTime + 1000 - 1) / 1000),
            (mTotalReceived >> 10),
            (mCurrentTime - mStartTime) / 1000
        );
    }
    return false;
}


void CNetServiceTCP::remove(CNetSession* iContext) {
    CAutoLock aulock(mMutex);
    ++mClosedSocket;

#if defined(APP_PLATFORM_WINDOWS)
    /*if(!mPoller.cancel(iContext->getSocket(), 0)) {
    IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::remove",
    "cancel IO, epoll ecode:[%d]", mPoller.getError());
    }*/
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    if(!mPoller.remove(iContext->getSocket())) {
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::remove",
            "ecode:[%d]", mPoller.getError());
    }
    iContext->getSocket().close();
#endif
    iContext->setTime(mCurrentTime);
    mWheel.remove(iContext->getTimeNode());
    mSessionPool.addIdleSession(iContext);

    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::remove",
        "[context:%u/%u][socket:%u/%u]",
        mSessionPool.getIdleCount(), mSessionPool.getMaxContext(),
        mClosedSocket, mCreatedSocket);
}


u32 CNetServiceTCP::receive(CNetSocket& sock, const CNetAddress& remote,
    const CNetAddress& local, INetEventer* evter) {
    if(!mRunning) {
        sock.close();
        return 0;
    }

    CAutoLock aulock(mMutex);

    CNetSession* session = mSessionPool.getIdleSession(mCurrentTime, APP_NET_SESSION_LINGER);
    if(!session) {
        sock.close();
        return 0;
    }

    bool success = sock.isOpen();
    if(success && sock.setBlock(false)) {
        success = false;
    }
    if(success && sock.setLinger(true, APP_NET_SESSION_LINGER / 1000)) {
        success = false;
    }
    /*if(success && sock.setReuseIP(true)) {
        success = false;
    }
    if(success && sock.setReusePort(true)) {
        success = false;
    }*/
    if(success && sock.setSendOvertime(APP_NET_SESSION_TIMEOUT)) {
        success = false;
    }
    if(success && sock.setReceiveOvertime(APP_NET_SESSION_TIMEOUT)) {
        success = false;
    }
    //if(success && sock.bind()) {
    //    success = false;
    //}
    if(!success) {
        sock.close();
        mSessionPool.addIdleSession(session);
        return 0;
    }
#if defined(APP_PLATFORM_WINDOWS)
    //don't add in iocp twice
    if(!mPoller.add(sock, reinterpret_cast<void*>(session->getIndex()))) {
        APP_ASSERT(0);
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::getSession",
            "can't add in epoll, socket: [%ld]",
            sock.getValue());

        sock.close();
        mSessionPool.addIdleSession(session->getIndex());
        return 0;
    }
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    CEventPoller::SEvent addevt;
    addevt.mData.mData32 = session->getIndex();
    addevt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET;
    if(!mPoller.add(sock, addevt)) {
        APP_ASSERT(0);
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::getSession",
            "can't add in epoll, socket: [%ld]",
            sock.getValue());

        sock.close();
        mSessionPool.addIdleSession(session->getIndex());
        return 0;
    }
#endif
    ++mCreatedSocket;
    session->clear();
    session->setHub(mID);
    session->setService(this);
    session->setSocket(sock);
    session->setTime(0);//busy session
    session->setLocalAddress(local);
    session->setRemoteAddress(remote);
    session->setEventer(evter);
    session->postEvent(ENET_LINKED);

    if(!activePoller(ENET_CMD_RECEIVE, session->getID())) {
        session->getSocket().close();
        mSessionPool.addIdleSession(session);
        return 0;
    }
    return session->getID();
}


u32 CNetServiceTCP::connect(const CNetAddress& remote, INetEventer* it) {
    if(!it || !mRunning) {
        return 0;
    }
    CAutoLock aulock(mMutex);

    CNetSession* session = mSessionPool.getIdleSession(mCurrentTime, APP_NET_SESSION_LINGER);
    if(!session) {
        return 0;
    }
    session->setTime(0);//busy session
    CNetSocket& sock = session->getSocket();
    if(!sock.isOpen()) { // a new node
#if defined(APP_PLATFORM_WINDOWS)
        bool success = sock.openSeniorTCP();
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        bool success = sock.openTCP();
#endif
        if(success && sock.setBlock(false)) {
            success = false;
        }
        if(success && sock.setLinger(true, APP_NET_SESSION_LINGER / 1000)) {
            success = false;
        }
        if(success && sock.setReuseIP(false)) {
            success = false;
        }
        if(success && sock.setReusePort(false)) {
            success = false;
        }
        if(success && sock.setSendOvertime(APP_NET_SESSION_TIMEOUT)) {
            success = false;
        }
        if(success && sock.setReceiveOvertime(APP_NET_SESSION_TIMEOUT)) {
            success = false;
        }
        if(success && sock.bind()) {
            success = false;
        }
        if(!success) {
            sock.close();
            mSessionPool.addIdleSession(session);
            return 0;
        }
#if defined(APP_PLATFORM_WINDOWS)
        //don't add in iocp twice
        if(!mPoller.add(sock, reinterpret_cast<void*>(session->getIndex()))) {
            APP_ASSERT(0);
            IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::getSession",
                "can't add in epoll, socket: [%ld]",
                sock.getValue());

            sock.close();
            mSessionPool.addIdleSession(session);
            return 0;
        }
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        CEventPoller::SEvent addevt;
        addevt.mData.mData32 = session->getIndex();
        addevt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET;
        if(!mPoller.add(sock, addevt)) {
            APP_ASSERT(0);
            IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::getSession",
                "can't add in epoll, socket: [%ld]",
                sock.getValue());

            sock.close();
            mSessionPool.addIdleSession(session);
            return 0;
        }
#endif
    }//if

     //APP_ASSERT(1);
    ++mCreatedSocket;
    session->clear();
    session->setHub(mID);
    session->setEventer(it);
    session->setService(this);
    CNetAddress local;
    sock.getLocalAddress(local);
    session->setLocalAddress(local);
    session->setRemoteAddress(remote);
    if(!activePoller(ENET_CMD_CONNECT, session->getID())) {
        session->getSocket().close();
        mSessionPool.addIdleSession(session);
        return 0;
    }
    return session->getID();
}


s32 CNetServiceTCP::send(u32 id, const void* buffer, s32 size) {
    APP_ASSERT(id > 0);
    if(mRunning && mQueueSend.lockPush(buffer, size, id)) {
        /*if(1 == AppAtomicIncrementFetch(&mLaunchedSendRequest)) {
            activePoller(ENET_CMD_SEND);
        }*/
        return size;
    }
    return 0;
}


bool CNetServiceTCP::activePoller(u32 cmd, u32 id/* = 0*/) {
    CEventPoller::SEvent evt;
#if defined(APP_PLATFORM_WINDOWS)
    evt.mPointer = 0;
    evt.mKey = ((ENET_CMD_MASK&cmd) | (ENET_SESSION_MASK&id));
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    evt.mData.mData64 = 0;
    evt.mEvent = ((ENET_CMD_MASK&cmd) | (ENET_SESSION_MASK&id));
#endif
    return mPoller.postEvent(evt);
}


s32 CNetServiceTCP::send(const u32* uid, u16 maxUID, const void* buffer, s32 size) {
    APP_ASSERT(uid && maxUID > 0);
    if(mRunning && mQueueSend.lockPush(buffer, size, uid, maxUID)) {
        /*if(1 == AppAtomicIncrementFetch(&mLaunchedSendRequest)) {
            activePoller(ENET_CMD_SEND);
        }*/
        return size;
    }
    return 0;
}


bool CNetServiceTCP::disconnect(u32 id) {
    APP_ASSERT(id > 0);
    CNetSession& nd = mSessionPool[id&ENET_SESSION_MASK];
    return nd.isValid(id) && activePoller(ENET_CMD_DISCONNECT, nd.getID());
}


void CNetServiceTCP::setEventer(u32 id, INetEventer* evt) {
    APP_ASSERT(id > 0);
    mSessionPool[id&ENET_SESSION_MASK].setEventer(evt);
}


#if defined(APP_DEBUG)
static s32 G_ENQUEUE_COUNT = 0;
static s32 G_DEQUEUE_COUNT = 0;
static s32 G_ERROR_COUNT = 0;
#endif

void CNetServiceTCP::addNetEvent(CNetSession& session) {
    CEventQueue::SNode* it = mQueueEvent.create(0);
    it->mEvent.mSessionID = session.getIndex();
    mQueueEvent.lockPush(it);
    bool ret = mThreadPool->addSoleTask(CNetServiceTCP::threadPoolCall, this);
    APP_ASSERT(ret);
#if defined(APP_DEBUG)
    AppAtomicIncrementFetch(&G_ENQUEUE_COUNT);
#endif
}


void CNetServiceTCP::threadPoolCall(void* it) {
    CNetServiceTCP& hub = *reinterpret_cast<CNetServiceTCP*>(it);
    CEventQueue::SNode* nd = hub.getEventQueue().lockPop();
    if(!nd) {
#if defined(APP_DEBUG)
        AppAtomicIncrementFetch(&G_ERROR_COUNT);
#endif
        APP_ASSERT(0);
        return;
    }
    hub.getSession(nd->mEvent.mSessionID).dispatchEvents();
    nd->mMemHub->release(nd);
#if defined(APP_DEBUG)
    AppAtomicIncrementFetch(&G_DEQUEUE_COUNT);
#endif
}


void CNetServiceTCP::dispatchBuffers() {
#if defined(APP_DEBUG)
    static u32 cnt = 0;
    static u32 cntf = 0;
    u32 bk = cnt;
    /*if(bk > 100) {
        mPoller.close();
    }*/
#endif
    if(mQueueSend.isEmpty()) {
        return;
    }
    u32 sessionID = 0;
    CBufferQueue::SBuffer* buf;
    for(s32 i = 0; i < 100; ++i) {
        buf = mQueueSend.lockPop();
        if(!buf) { break; }
        for(; buf->mSessionCount < buf->mSessionMax; ++buf->mSessionCount) {
            sessionID = buf->getSessionID(buf->mSessionCount);
            CNetSession& session = getSession(ENET_SESSION_MASK & sessionID);
            if(session.postSend(sessionID, buf) < 0) {
                //error
                buf->drop();//TODO>> callback
#if defined(APP_DEBUG)
                ++cntf;
#endif
            }
#if defined(APP_DEBUG)
            ++cnt;
#endif
        }//for
    }//for

#if defined(APP_DEBUG)
    if(cntf>0 && cnt != bk) {
        APP_LOG(ELOG_INFO, "CNetServiceTCP::dispatchBuffers", "count=%u/%u", cntf, ++cnt);
    }
#endif
}


void CNetServiceTCP::clearBuffers() {
    u32 cnt = 0;
    CBufferQueue::SBuffer* buf = mQueueSend.lockPop();
    for(; buf; buf = mQueueSend.lockPop()) {
        buf->drop();
        ++cnt;
    }
    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::clearBuffers", "count=%u", cnt);
}

}//namespace net
}//namespace irr
