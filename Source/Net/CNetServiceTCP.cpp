#include "CNetServiceTCP.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "CMemoryHub.h"
#include "CNetUtility.h"

#define APP_NET_SESSION_TIMEOUT     5000
#define APP_NET_SESSION_LINGER      8000


namespace irr {
namespace net {

CNetServiceTCP::CNetServiceTCP(CNetConfig* cfg) :
    mID(0),
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
    mTimeInterval(cfg->mPollTimeout) {
    CNetUtility::loadSocketLib();
    APP_ASSERT(cfg);
    cfg->grab();
    mConfig = cfg;
}


CNetServiceTCP::~CNetServiceTCP() {
    stop();
    CNetUtility::unloadSocketLib();
    if(mConfig) {
        mConfig->drop();
        mConfig = 0;
    }
}



#if defined(APP_PLATFORM_WINDOWS)
void CNetServiceTCP::run() {
    const u32 maxe = mConfig->mMaxFetchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    CNetSession* iContext;
    SContextIO* iAction = 0;
    const u64 tmgap = mWheel.getInterval();
    u64 lastshow = mCurrentTime;
    u64 last = mCurrentTime;
    u32 gotsz = 0;
    mWheel.setCurrent(static_cast<u32>(mCurrentTime));
    u32 action;
    s32 ret;
    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, mTimeInterval);
        if(gotsz > 0) {
            mCurrentTime = IAppTimer::getTime();
            for(u32 i = 0; i < gotsz; ++i) {
                ret = 2;
                if(iEvent[i].mKey < ENET_SESSION_MASK && iEvent[i].mPointer) {
                    iContext = mSessionPool.getSession(iEvent[i].mKey);
                    iAction = APP_GET_VALUE_POINTER(iEvent[i].mPointer, SContextIO, mOverlapped);
                    iAction->mBytes = iEvent[i].mBytes;
                    mTotalReceived += iEvent[i].mBytes;

                    switch(iAction->mOperationType) {
                    case EOP_SEND:
                        ret = iContext->stepSend(*iAction);
                        break;

                    case EOP_RECEIVE:
                        ret = iContext->stepReceive(*iAction);
                        break;

                    case EOP_CONNECT:
                        ret = iContext->stepConnect(*iAction);
                        break;

                    case EOP_DISCONNECT:
                        ret = iContext->stepDisonnect(*iAction);
                        break;

                    default:
                        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::run",
                            "unknown operation: [%u]", iAction->mOperationType);
                        continue;
                    }//switch
                } else {
                    APP_ASSERT(0 == iEvent[i].mPointer);
                    if(ENET_SESSION_MASK == iEvent[i].mKey) {
                        mRunning = false;
                        continue;
                    }
                    action = ENET_CMD_MASK & iEvent[i].mKey;
                    iContext = mSessionPool.getSession(ENET_SESSION_MASK & iEvent[i].mKey);

                    switch(action) {
                    case ENET_CMD_SEND:
                        //AppAtomicDecrementFetch(&mLaunchedSendRequest);
                        ret = -1;
                        break;
                    case ENET_CMD_RECEIVE:
                        ret = iContext->postReceive();
                        break;
                    case ENET_CMD_CONNECT:
                        ret = iContext->postConnect();
                        break;
                    case ENET_CMD_DISCONNECT:
                        ret = iContext->postDisconnect();
                        break;
                    case ENET_CMD_TIMEOUT:
                        ret = iContext->stepTimeout();
                        break;
                    default:
                        //APP_ASSERT(0);
                        ret = -1;
                        break;
                    }//switch
                }//if

                if(ret > 0) {
                    mWheel.add(iContext->getTimeNode(),
                        ret > 1 ? mConfig->mSessionTimeout : 1000, 1);
                } else if(0 == ret) {
                    iContext->stepClose();             //step 1
                    remove(iContext);                  //step 2
                }
            }//for
            if(mCurrentTime - last >= tmgap) {
                last = mCurrentTime;
                mWheel.update(mCurrentTime);
            }
        } else {//poller error
            s32 pcode = mPoller.getError();
            bool bigerror = false;
            switch(pcode) {
            case WAIT_TIMEOUT:
                mCurrentTime = IAppTimer::getTime();
                mWheel.update(mCurrentTime);
                last = mCurrentTime;
                if(mCurrentTime - lastshow >= 1000) {
                    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run",
                        "[context:%u/%u][socket:%u/%u]",
                        mSessionPool.getIdleCount(), mSessionPool.getMaxContext(),
                        mClosedSocket, mCreatedSocket);
                    lastshow = mCurrentTime;
                }
                break;

            case ERROR_ABANDONED_WAIT_0: //socket closed
            default:
                IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::run",
                    "invalid overlap, ecode: [%lu]", pcode);
                APP_ASSERT(0);
                bigerror = true;
                break;
            }//switch
            if(bigerror) {
                break;
            }
        }//if

        dispatchBuffers();
    }//for

    delete[] iEvent;
    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run", "thread exited");
}

//void CNetServiceTCP::postSend() {
//    for(u32 i = 0; i < mConfig->mMaxSendMsg; ) {
//        CBufferQueue::SBuffer* buf = mQueueSend.getHead();
//        if(0 == buf) { break; }
//        if(buf->mSessionCount >= buf->mSessionMax) {
//            ++i;
//            mQueueSend.pop();
//            continue;
//        }
//        for(; buf->mSessionCount < buf->mSessionMax; ++buf->mSessionCount) {
//            mSessionPool[buf->getSessionID(buf->mSessionCount)].postSend();
//            ++i;
//        }
//        mQueueSend.pop();
//    }
//}


bool CNetServiceTCP::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    case WSA_IO_PENDING:
        return true;
    case ERROR_SEM_TIMEOUT: //hack? 每秒收到5000个以上的Accept时出现
        return true;
    case WAIT_TIMEOUT: //same as WSA_WAIT_TIMEOUT
        break;
    case ERROR_CONNECTION_ABORTED: //服务器主动关闭
    case ERROR_NETNAME_DELETED: //客户端主动关闭连接
    {
        IAppLogger::log(ELOG_INFO, "CNetServiceTCP::clearError",
            "client quit abnormally, [ecode=%u]", ecode);
        //removeClient(pContext);
        return true;
    }
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch

    return false;
}


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

void CNetServiceTCP::run() {
    CNetSocket sock(mPoller.getSocketPair().getSocketA());
    const u32 maxe = mConfig->mMaxFetchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    CNetSession* iContext;
    u64 last = mCurrentTime;
    u32 gotsz = 0;
    mWheel.setCurrent(mCurrentTime);
    u32 action;
    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, mTimeInterval);
        if(gotsz > 0) {
            mCurrentTime = IAppTimer::getTime();
            for(u32 i = 0; i < gotsz; ++i) {
                s32 ret = 1;
                if(iEvent[i].mData.mData32 < ENET_SESSION_MASK) {
                    iContext = mSessionPool.getSession(iEvent[i].mData.mData32);
                    //mTotalReceived += iEvent[i].mBytes;
                    if(EPOLLIN & iEvent[i].mEvent) {
                        ret = iContext->stepSend();
                        ret = iContext->stepReceive();
                        ret = iContext->stepConnect();
                        ret = iContext->stepDisonnect();
                        if(0 == ret) {
                            remove(iContext);
                            //continue;
                        } else {
                            APP_LOG(ELOG_ERROR, "CNetServiceTCP::run",
                                "stepDisonnect: [%d]", ret);
                        }
                    } else if(EPOLLOUT & iEvent[i].mEvent) {

                    }
                } else {
                    u32 mask;
                    s32 ret = sock.receiveAll(&mask, sizeof(mask));
                    if(ret != sizeof(mask)) {
                        continue;
                    }
                    if(ENET_SESSION_MASK == mask) {
                        mRunning = false;
                        break;
                    }
                    action = ENET_CMD_MASK & mask;
                    iContext = mSessionPool.getSession(ENET_SESSION_MASK & mask);

                    switch(action) {
                    case ENET_CMD_SEND:
                        ret = iContext->postSend();
                        break;
                    case ENET_CMD_RECEIVE:
                        //just add in time wheel
                        break;
                    case ENET_CMD_CONNECT:
                        ret = iContext->postConnect();
                        break;
                    case ENET_CMD_DISCONNECT:
                        ret = iContext->postDisconnect();
                        break;
                    case ENET_CMD_TIMEOUT:
                        ret = iContext->postDisconnect();
                        if(ret < 0) {//failed on second time or error
                            iContext->stepDisonnect();             //step 1
                            iContext->getSocket().close();         //step 2
                            remove(iContext);                      //step 3
                            ++mClosedSocket;
                            //continue;
                        }
                        break;
                    }//switch
                }//if

                if(ret > 0) {
                    mWheel.add(iContext->getTimeNode(), 2 * mTimeInterval, 1);
                }
            }//for

            if(mCurrentTime - last > mTimeInterval) {
                last = mCurrentTime;
                mWheel.update(mCurrentTime);
                IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run",
                    "[context:%u/%u][socket:%u/%u][total:%u]",
                    mSessionPool.getIdleCount(), mSessionPool.getMaxContext(),
                    mClosedSocket, mCreatedSocket,
                    mTotalSession);
            }
        } else {//poller error
            s32 pcode = mPoller.getError();
            mCurrentTime = IAppTimer::getTime();
            mWheel.update(mCurrentTime);
            last = mCurrentTime;
            //clearError(iContext, iAction);
            continue;
        }//if
    }//for

    delete[] iEvent;
    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run", "thread exited");
}


bool CNetServiceTCP::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::clearError",
            "operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch

    return false;
}

#endif //APP_PLATFORM_LINUX



bool CNetServiceTCP::start() {
    if(mRunning) {
        return true;
    }
#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    CNetSocket& sock = mPoller.getSocketPair().getSocketA();
    if(!sock.isOpen()) {
        //printf("error %d on socketpair\n", errno);
        return false;
    }
    CEventPoller::SEvent evt;
    evt.mData.mData32 = ENET_SESSION_MASK;
    evt.mEvent = EPOLLIN;
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
    addevt.mData.mPointer = session;
    addevt.mEvent = EPOLLIN | EPOLLOUT;
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
        addevt.mData.mPointer = &session;
        addevt.mEvent = EPOLLIN | EPOLLOUT;
        if(!mPoller.add(sock, addevt)) {
            APP_ASSERT(0);
            IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::getSession",
                "can't add in epoll, socket: [%ld]",
                sock.getValue());

            sock.close();
            mSessionPool.addIdleSession(&session);
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
    if(cnt != bk) {
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
