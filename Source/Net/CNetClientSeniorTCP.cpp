#include "CNetClientSeniorTCP.h"
#include "CNetUtility.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "CThread.h"
#include "INetEventer.h"
#include "CNetSession.h"
#include "IAppTimer.h"


#define APP_NET_SESSION_TIMEOUT     10000
#define APP_NET_SESSION_LINGER       5000


namespace irr {
namespace net {

//thread function
//void AppTimeoutThreadCallback(void* it) {
//    APP_ASSERT(it);
//    CNetClientSeniorTCP* nethub = (CNetClientSeniorTCP*) it;
//    while(nethub->isRunning()) {
//        CThread::sleep(nethub->updateTimeWheel());
//    }
//}

//u32 CNetClientSeniorTCP::updateTimeWheel() {
//    mWheel.update(mCurrentTime);
//    return mTimeInterval;
//}


CNetClientSeniorTCP::CNetClientSeniorTCP() :
    mWheel(0, 200),//APP_NET_SESSION_TIMEOUT/5
    mTotalReceived(0),
    mTimeInterval(APP_NET_SESSION_TIMEOUT),
    mAllContext(0),
    mCurrentTime(0),
    mMutex(0),
    mNetThread(0),
    mTotalSession(0),
    mClosedSocket(0),
    mCreatedSocket(0),
    mCreatedSession(0),
    mRunning(false) {

    CNetUtility::loadSocketLib();
}


CNetClientSeniorTCP::~CNetClientSeniorTCP() {
    CNetUtility::unloadSocketLib();
    stop();
}


u32 CNetClientSeniorTCP::getReuseRate() {
    f32 percent = (f32) (mTotalSession - mClosedSocket);
    percent /= mTotalSession;
    return u32(100.f * percent + 0.5f);
}


u32 CNetClientSeniorTCP::getCreatedSocket() {
    return mCreatedSocket;
}


u32 CNetClientSeniorTCP::getTotalSession() {
    return mTotalSession;
}


u32 CNetClientSeniorTCP::getClosedSocket() {
    return mClosedSocket;
}


void CNetClientSeniorTCP::clearError(CNetSession* iContext, SContextIO* iAction) {
    //CAutoLock aulock(*mMutex);
    s32 ecode = CNetSocket::getError();
#if defined(APP_PLATFORM_WINDOWS)
    switch(ecode) {
    case WSAENOTCONN:               //10057
    case WSAECONNREFUSED:           //10061
    case ERROR_CONNECTION_REFUSED:  //1225
        //因超时主动关闭socket, post disconnect成功， 下次也会返回ERROR_CONNECTION_REFUSED。
        break;
    case ERROR_SEM_TIMEOUT:         //121
        break;                      //received 0 bytes
    case ERROR_NETNAME_DELETED:     //64, 被动关闭
        break;
    case ERROR_CONNECTION_ABORTED:  //1236, 主动关闭socket close.
        break;
    case WSA_OPERATION_ABORTED:     //995, 主动关闭socket
        break;
    case WSAECONNRESET:             //10054
        break;
    case WSAENOBUFS:                //10055
        break;
    case WSAETIMEDOUT:              //10060
        break;
    case ERROR_DUP_NAME:            //52
        break;
    case ERROR_INVALID_NETNAME: //1214
        break;
    default:
        IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::clearError", "default ecode:[%u]", ecode);
        break;
    }//switch
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    switch(ecode) {
    case 0:
        break;
    }//switch
#endif
    APP_LOG(ELOG_ERROR, "CNetClientSeniorTCP::clearError", "[ecode:%u]", ecode);
}

//IO thread
void CNetClientSeniorTCP::run() {
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
                            APP_LOG(ELOG_ERROR, "CNetClientSeniorTCP::run",
                                "stepDisonnect: [%d]", ret);
                        }
                        break;

                    default:
                        IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::run",
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
                IAppLogger::log(ELOG_INFO, "CNetClientSeniorTCP::run",
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
                IAppLogger::log(ELOG_INFO, "CNetClientSeniorTCP::run",
                    "[context:%u/%u][socket:%u/%u][total:%u]",
                    mIdleSession.size(), mCreatedSession,
                    mClosedSocket, mCreatedSocket,
                    mTotalSession);
                continue;

            case ERROR_ABANDONED_WAIT_0: //socket closed
                APP_ASSERT(0);
                continue;

            default:
                IAppLogger::log(ELOG_WARNING, "CNetClientSeniorTCP::run",
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


void CNetClientSeniorTCP::createContext(u32 max) {
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


void CNetClientSeniorTCP::deleteContext() {
    for(u32 i = 0; i < mCreatedSession; ++i) {
        //mAllContext[i].~CNetSession();
        mAllContext[i].getSocket().close();
    }
    delete[] mAllContext;
    mAllContext = 0;
    mCreatedSession = 0;
}


bool CNetClientSeniorTCP::start() {
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
    mMutex = new CMutex();
    mNetThread = new CThread();
    mNetThread->start(*this);
    return true;
}


bool CNetClientSeniorTCP::stop() {
    if(!mRunning) {
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
        mNetThread->join();
        deleteContext();
        delete mNetThread;
        delete mMutex;
        return true;
    }
    return false;
}


INetSession* CNetClientSeniorTCP::getSession(INetEventer* it) {
    if(!it) return 0;

    CAutoLock aulock(*mMutex);

    if(0 == mIdleSession.size()) {
        return 0;
    }
    core::list<u32>::Iterator first = mIdleSession.begin();
    CNetSession& session = mAllContext[*first];
    if(mCurrentTime <= (session.getTime() + APP_NET_SESSION_LINGER)) {
        return 0;
    }
    session.setTime(0);//busy session
    mIdleSession.erase(first);
    CNetSocket& sock = session.getSocket();
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
            IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::getSession",
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
            IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::getSession",
                "can't add in epoll, socket: [%ld]",
                sock.getValue());

            sock.close();
            mIdleSession.push_back(session.getIndex());
            return 0;
        }
#endif
        ++mCreatedSocket;
    }//if

    //APP_ASSERT(1);
    ++mTotalSession;
    session.clear();
    session.setEventer(it);
    if(!session.onNewSession()) {
        mIdleSession.push_back(session.getIndex());
        return 0;
    }
    return &session;
}


void CNetClientSeniorTCP::remove(CNetSession* iContext) {
    if(iContext->getTime()) {
        return;
    }
    CAutoLock aulock(*mMutex);
#if defined(APP_PLATFORM_WINDOWS)
    /*if(!mPoller.cancel(iContext->getSocket(), 0)) {
        IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::remove",
            "cancel IO, epoll ecode:[%d]", mPoller.getError());
    }*/
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    if(!mPoller.remove(iContext->getSocket())) {
        IAppLogger::log(ELOG_ERROR, "CNetClientSeniorTCP::remove",
            "ecode:[%d]", mPoller.getError());
    }
    iContext->getSocket().close();
#endif
    iContext->setTime(mCurrentTime);
    mWheel.remove(iContext->getTimeNode());
    mIdleSession.push_back(iContext->getIndex());
}


} //namespace net
} //namespace irr