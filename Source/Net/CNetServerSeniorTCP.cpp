#include "CNetServerSeniorTCP.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
//#include "CNetSession.h"
//#include "IUtility.h"
//#include "CNetUtility.h"


#define APP_SERVER_EXIT_CODE  0
#define APP_DEFAULT_CLIENT_CACHE (10)
#define APP_NET_SESSION_TIMEOUT     5000
#define APP_NET_SESSION_LINGER      8000


namespace irr {
namespace net {

CNetServerSeniorTCP::CNetServerSeniorTCP(CNetConfig* cfg) :
    mThread(0),
    mCreatedSocket(0),
    mClosedSocket(0),
    mTotalSession(0),
    mTotalReceived(0),
    mWheel(0, 200),//APP_NET_SESSION_TIMEOUT/5
    mRunning(false),
    mReceiver(0),
    mStartTime(0),
    mCurrentTime(0),
    mTimeInterval(cfg->mPollTimeout) {
    //CNetUtility::loadSocketLib();
    APP_ASSERT(cfg);
    cfg->grab();
    mConfig = cfg;
}


CNetServerSeniorTCP::~CNetServerSeniorTCP() {
    stop();
    //CNetUtility::unloadSocketLib();
    if(mConfig) {
        mConfig->drop();
        mConfig = 0;
    }
}



#if defined(APP_PLATFORM_WINDOWS)
void CNetServerSeniorTCP::run() {
    const u32 maxe = mConfig->mMaxFatchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    CNetSession* iContext;
    SContextIO* iAction = 0;
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
                ret = 1;
                if(iEvent[i].mKey < ENET_SESSION_MASK) {
                    iContext = mSessionPool.getSession(iEvent[i].mKey);
                    iAction = APP_GET_VALUE_POINTER(iEvent[i].mPointer, SContextIO, mOverlapped);
                    iAction->mBytes = iEvent[i].mBytes;
                    mTotalReceived += iEvent[i].mBytes;

                    switch(iAction->mOperationType) {
                    case EOP_SEND:
                        ret = iContext->stepSend();
                        break;

                    case EOP_RECEIVE:
                        ret = iContext->stepReceive();
                        break;

                    case EOP_CONNECT:
                        ret = iContext->stepConnect();
                        break;

                    case EOP_DISCONNECT:
                        ret = iContext->stepDisonnect();
                        if(0 == ret) {
                            remove(iContext);
                            //continue;
                        } else {
                            APP_LOG(ELOG_ERROR, "CNetServerSeniorTCP::run",
                                "stepDisonnect: [%d]", ret);
                        }
                        break;

                    default:
                        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::run",
                            "unknown operation: [%u]", iAction->mOperationType);
                        continue;
                    }//switch
                } else {
                    if(ENET_SESSION_MASK == iEvent[i].mKey) {
                        mRunning = false;
                        continue;
                    }
                    action = ENET_CMD_MASK & iEvent[i].mKey;
                    iContext = mSessionPool.getSession(ENET_SESSION_MASK & iEvent[i].mKey);

                    switch(action) {
                    case ENET_CMD_SEND:
                        ret = iContext->postSend();
                        break;
                    case ENET_CMD_NEW_SESSION:
                        if(iContext->getEventer()) {
                            SNetEvent evt;
                            evt.mType = ENET_LINKED;
                            evt.mInfo.mSession.mSocket = &iContext->getSocket();
                            evt.mInfo.mSession.mAddressLocal = &iContext->getLocalAddress();
                            evt.mInfo.mSession.mAddressRemote = &iContext->getRemoteAddress();
                            evt.mInfo.mSession.mContext = iContext;
                            iContext->getEventer()->onEvent(evt);
                        }
                        ret = iContext->postReceive();
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

                if(ret >= 0) {
                    mWheel.add(iContext->getTimeNode(), 2 * mTimeInterval);
                }
            }//for
            if(mCurrentTime - last > mTimeInterval) {
                last = mCurrentTime;
                mWheel.update(static_cast<u32>(mCurrentTime));
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run",
                    "[context:%u/%u][socket:%u/%u][total:%u]",
                    mSessionPool.getIdleCount(), mSessionPool.getMaxContext(),
                    mClosedSocket, mCreatedSocket,
                    mTotalSession);
            }
        } else {//poller error
            s32 pcode = mPoller.getError();
            switch(pcode) {
            case WAIT_TIMEOUT:
                //mCurrentTime += mTimeInterval;
                mCurrentTime = IAppTimer::getTime();
                mWheel.update(static_cast<u32>(mCurrentTime));
                last = mCurrentTime;
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run",
                    "[context:%u/%u][socket:%u/%u][total:%u]",
                    mSessionPool.getIdleCount(), mSessionPool.getMaxContext(),
                    mClosedSocket, mCreatedSocket,
                    mTotalSession);
                continue;

            case ERROR_ABANDONED_WAIT_0: //socket closed
                APP_ASSERT(0);
                continue;

            default:
                IAppLogger::log(ELOG_WARNING, "CNetServerSeniorTCP::run",
                    "invalid overlap, ecode: [%lu]", pcode);
                APP_ASSERT(0);
                continue;
            }//switch
            //clearError(iContext, iAction);
            continue;
        }//if
    }//for

    delete[] iEvent;
    IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run", "thread exited");
}


bool CNetServerSeniorTCP::clearError() {
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
        IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::clearError",
            "client quit abnormally, [ecode=%u]", ecode);
        //removeClient(pContext);
        return true;
    }
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch

    return false;
}

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
void CNetServerSeniorTCP::run() {
    CNetSocket sock(mPoller.getSocketPair().getSocketA());
    const u32 maxe = 20;
    CEventPoller::SEvent iEvent[maxe];
    CNetSession* iContext;
    u64 last = mCurrentTime;
    u32 gotsz = 0;
    mWheel.setCurrent(static_cast<u32>(mCurrentTime));
    u32 action;
    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, mTimeInterval);
        if(gotsz > 0) {
            mCurrentTime = IAppTimer::getTime();
            for(u32 i = 0; i < gotsz; ++i) {
                s32 ret = 1;
                if(iEvent[i].mData.mData32 < ENET_SESSION_MASK) {
                    iContext = &mAllContext[iEvent[i].mData.mData32];
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
                            APP_LOG(ELOG_ERROR, "CNetServerSeniorTCP::run",
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
                    iContext = &mAllContext[ENET_SESSION_MASK & mask];

                    switch(action) {
                    case ENET_CMD_SEND:
                        ret = iContext->postSend();
                        break;
                    case ENET_CMD_NEW_SESSION:
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
                    mWheel.add(iContext->getTimeNode(), 2 * mTimeInterval);
                }
            }//for

            if(mCurrentTime - last > mTimeInterval) {
                last = mCurrentTime;
                mWheel.update(static_cast<u32>(mCurrentTime));
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run",
                    "[context:%u/%u][socket:%u/%u][total:%u]",
                    mIdleSession.size(), mCreatedSession,
                    mClosedSocket, mCreatedSocket,
                    mTotalSession);
            }
        } else {//poller error
            s32 pcode = mPoller.getError();
            mCurrentTime = IAppTimer::getTime();
            mWheel.update(static_cast<u32>(mCurrentTime));
            last = mCurrentTime;
            //clearError(iContext, iAction);
            continue;
        }//if
    }//for
}


bool CNetServerSeniorTCP::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::clearError",
            "operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch

    return false;
}

#endif //APP_PLATFORM_LINUX



bool CNetServerSeniorTCP::start() {
    if(mRunning) {
        return true;
    }
#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    CNetSocket sock(mPoller.getSocketPair().getSocketA());
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
    mRunning = true;
    mTotalReceived = 0;
    mClosedSocket = 0;
    mCurrentTime = IAppTimer::getTime();
    mStartTime = mCurrentTime;
    mSessionPool.create(mConfig->mMaxContext);
    mThread = new CThread();
    mThread->start(*this);
    return true;
}


bool CNetServerSeniorTCP::stop() {
    if(!mRunning) {
        //IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stop", "server had stoped.");
        return true;
    }

    CEventPoller::SEvent evt;
    evt.setMessage(ENET_SESSION_MASK);
    mCurrentTime = IAppTimer::getTime();
    if(mPoller.postEvent(evt)) {
        mRunning = false;
        mThread->join();
        delete mThread;
        mThread = 0;
        IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stop",
            "Statistics: [session=%u][speed=%luKb/s][size=%uKb][seconds=%lu]",
            mTotalSession,
            (mTotalReceived >> 10) / ((mCurrentTime - mStartTime) / 1000),
            (mTotalReceived >> 10),
            (mCurrentTime - mStartTime) / 1000
        );
        return true;
    }

    //APP_LOG(ELOG_INFO, "CNetServerSeniorTCP::stop", "server stoped success");
    return false;
}


void CNetServerSeniorTCP::remove(CNetSession* iContext) {
    if(iContext->getTime()) {
        return;
    }
    CAutoLock aulock(mMutex);
#if defined(APP_PLATFORM_WINDOWS)
    /*if(!mPoller.cancel(iContext->getSocket(), 0)) {
    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::remove",
    "cancel IO, epoll ecode:[%d]", mPoller.getError());
    }*/
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    if(!mPoller.remove(iContext->getSocket())) {
        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::remove",
            "ecode:[%d]", mPoller.getError());
    }
    iContext->getSocket().close();
#endif
    iContext->setTime(mCurrentTime);
    mWheel.remove(iContext->getTimeNode());
    mSessionPool.addIdleSession(iContext);
}


CNetSession* CNetServerSeniorTCP::addSession(CNetSocket& sock, const CNetAddress& remote,
    const CNetAddress& local, INetEventer* evter) {
    CAutoLock aulock(mMutex);

    if(0 == mSessionPool.getIdleCount()) {
        return 0;
    }
    CNetSession& session = *mSessionPool.getIdleSession();
    if(mCurrentTime <= (session.getTime() + APP_NET_SESSION_LINGER)) {
        mSessionPool.addIdleSession(&session);
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
        mSessionPool.addIdleSession(&session);
        return 0;
    }
#if defined(APP_PLATFORM_WINDOWS)
    //don't add in iocp twice
    if(!mPoller.add(sock, reinterpret_cast<void*>(session.getIndex()))) {
        APP_ASSERT(0);
        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::getSession",
            "can't add in epoll, socket: [%ld]",
            sock.getValue());

        sock.close();
        mSessionPool.addIdleSession(session.getIndex());
        return 0;
    }
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    CEventPoller::SEvent addevt;
    addevt.mData.mPointer = &session;
    addevt.mEvent = EPOLLIN | EPOLLOUT;
    if(!mPoller.add(sock, addevt)) {
        APP_ASSERT(0);
        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::getSession",
            "can't add in epoll, socket: [%ld]",
            sock.getValue());

        sock.close();
        mIdleSession.push_back(session.getIndex());
        return 0;
    }
#endif
    ++mCreatedSocket;
    ++mTotalSession;
    session.clear();
    session.setPoller(&mPoller);
    session.setSocket(sock);
    session.setTime(0);//busy session
    session.getLocalAddress() = local;
    session.getRemoteAddress() = remote;
    session.setEventer(evter);
    if(!session.onNewSession()) {
        mSessionPool.addIdleSession(&session);
        return 0;
    }
    return &session;
}


}//namespace net
}//namespace irr
