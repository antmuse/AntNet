#include "CNetServerSeniorTCP.h"
#include "CNetSession.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
//#include "IUtility.h"
//#include "CNetUtility.h"


#define APP_SERVER_EXIT_CODE  0
#define APP_DEFAULT_CLIENT_CACHE (10)
#define APP_NET_SESSION_TIMEOUT     10000
#define APP_NET_SESSION_LINGER       5000


namespace irr {
namespace net {

CNetServerSeniorTCP::CNetServerSeniorTCP() :
    mMaxContext(10000),
    mThread(0),
    mAllContext(0),
    mWheel(0, 200),//APP_NET_SESSION_TIMEOUT/5
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mRunning(false),
    mReceiver(0),
    mStartTime(0),
    mCurrentTime(0),
    mTimeInterval(APP_NET_TICK_TIME) {
    //CNetUtility::loadSocketLib();
}


CNetServerSeniorTCP::~CNetServerSeniorTCP() {
    stop();
    //CNetUtility::unloadSocketLib();
}


u32 CNetServerSeniorTCP::getClientCount()const {
    return mMaxContext - mIdleSession.size();
}


#if defined(APP_PLATFORM_WINDOWS)
void CNetServerSeniorTCP::run() {
    const u32 maxe = 20;
    CEventPoller::SEvent iEvent[maxe];
    CNetSession* iContext;
    SContextIO* iAction = 0;
    u64 last = mCurrentTime;
    u32 gotsz = 0;
    mWheel.setCurrent(static_cast<u32>(mCurrentTime));
    u32 action;
    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, mTimeInterval);
        if(gotsz > 0) {
            mCurrentTime = IAppTimer::getTime();

#if defined(APP_PLATFORM_WINDOWS)
            for(u32 i = 0; i < gotsz; ++i) {

                s32 ret = 1;

                if(iEvent[i].mKey < ENET_SESSION_MASK) {
                    iContext = &mAllContext[iEvent[i].mKey];
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
                        break;
                    }
                    action = ENET_CMD_MASK & iEvent[i].mKey;
                    iContext = &mAllContext[ENET_SESSION_MASK & iEvent[i].mKey];

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
                    mWheel.add(iContext->getTimeNode(), mTimeInterval);
                }
            }//for

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

#endif
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
#if defined(APP_PLATFORM_WINDOWS)
            switch(pcode) {
            case WAIT_TIMEOUT:
                //mCurrentTime += mTimeInterval;
                mCurrentTime = IAppTimer::getTime();
                mWheel.update(static_cast<u32>(mCurrentTime));
                last = mCurrentTime;
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run",
                    "[context:%u/%u][socket:%u/%u][total:%u]",
                    mIdleSession.size(), mCreatedSession,
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

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#endif
            //因超时主动关闭socket, epoll return false, 此时iEvent.mKey & iEvent.mPointer有效。
            //clearError(iContext, iAction);
            continue;
        }//if
    }//for
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
                    if(ENET_SESSION_MASK == iEvent[i].mData.mData32) {
                        mRunning = false;
                        break;
                    }
                    action = ENET_CMD_MASK & iEvent[i].mData.mData32;
                    iContext = &mAllContext[ENET_SESSION_MASK & iEvent[i].mData.mData32];

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
                    mWheel.add(iContext->getTimeNode(), mTimeInterval);
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




void CNetServerSeniorTCP::createContext(u32 max) {
    APP_ASSERT(max < ENET_SESSION_MASK);
    if(0 == mAllContext) {
        mAllContext = new CNetSession[max];
        for(u32 i = 0; i < max; ++i) {
            //new (&mAllContext[i]) CNetSession();
            mAllContext[i].setIndex(i);
            //mAllContext[i].setSessionHub(this);
            mAllContext[i].setPoller(&mPoller);
            mAllContext[i].setTime(-1);
            mIdleSession.push_back(i);
        }
    }
    mCreatedSession = max;
}


void CNetServerSeniorTCP::deleteContext() {
    for(u32 i = 0; i < mCreatedSession; ++i) {
        //mAllContext[i].~CNetSession();
        mAllContext[i].getSocket().close();
    }
    delete[] mAllContext;
    mAllContext = 0;
    mCreatedSession = 0;
}



bool CNetServerSeniorTCP::start() {
    if(mRunning) {
        return true;
    }
    mRunning = true;
    mCreatedSession = 0;
    mTotalReceived = 0;
    mClosedSocket = 0;
    mCurrentTime = IAppTimer::getTime();
    mStartTime = mCurrentTime;
    createContext(mMaxContext);
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
#if defined(APP_PLATFORM_WINDOWS)
    evt.mBytes = 0;
    evt.mKey = ENET_SESSION_MASK;
    evt.mPointer = 0;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    evt.mData.mData64 = 0;
    evt.mEvent = ENET_SESSION_MASK;
#endif

    mCurrentTime = IAppTimer::getTime();
    if(mPoller.postEvent(evt)) {
        mRunning = false;
        mThread->join();
        deleteContext();
        delete mThread;
        mThread = 0;
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
    mIdleSession.push_back(iContext->getIndex());
}


CNetSession* CNetServerSeniorTCP::addSession(CNetSocket& sock, const SNetAddress& remote, const SNetAddress& local) {
    CAutoLock aulock(mMutex);

    if(0 == mIdleSession.size()) {
        return 0;
}
    core::list<u32>::Iterator first = mIdleSession.begin();
    CNetSession& session = mAllContext[*first];
    if(mCurrentTime <= (session.getTime() + APP_NET_SESSION_LINGER)) {
        return 0;
    }
    mIdleSession.erase(first);

    bool success = sock.isOpen();
    if(success && sock.setBlock(false)) {
        success = false;
    }
    if(success && sock.setLinger(true, APP_NET_SESSION_LINGER / 1000)) {
        success = false;
    }
    if(success && sock.setReuseIP(true)) {
        success = false;
    }
    if(success && sock.setReusePort(true)) {
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
        mIdleSession.push_back(session.getIndex());
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
        mIdleSession.push_back(session.getIndex());
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
    session.setTime(0);//busy session
    session.getLocalAddress() = local;
    session.getRemoteAddress() = remote;
    session.setEventer(mReceiver);
    if(!session.onNewSession()) {
        mIdleSession.push_back(session.getIndex());
        return 0;
    }
    return &session;
    }


}//namespace net
}//namespace irr
