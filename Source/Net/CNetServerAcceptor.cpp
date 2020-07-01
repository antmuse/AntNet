#include "CNetServerAcceptor.h"
#include "CNetUtility.h"
#include "CTimer.h"
#include "CLogger.h"
#include "SClientContext.h"


///The infomation to inform workers quit.
#define APP_SERVER_EXIT_CODE  0

#define APP_THREAD_MAX_SLEEP_TIME  0xffffffff


#if defined(APP_PLATFORM_WINDOWS)
namespace app {
namespace net {


CNetServerAcceptor::SContextWaiter::SContextWaiter() {
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(sizeof(mCache) == 2 * (sizeof(sockaddr_in6) + 16));
#else
    APP_ASSERT(sizeof(mCache) == 2 * (sizeof(sockaddr_in) + 16));
#endif
    mContextIO = new SContextIO();
}

CNetServerAcceptor::SContextWaiter::~SContextWaiter() {
    delete mContextIO;
    mSocket.close();
}

bool CNetServerAcceptor::SContextWaiter::reset() {
    mSocket = APP_INVALID_SOCKET;
    return mSocket.openSeniorTCP();
}


/////////////////////////////////////////////////////////////////////////////////////
CNetServerAcceptor::CNetServerAcceptor() :
    mReceiver(0),
    mAcceptCount(0),
    mThread(0),
    mCreated(false),
    mService(nullptr),
    mConfig(nullptr),
    mAllWaiter(8),
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetServerAcceptor::~CNetServerAcceptor() {
    stop();
    CNetUtility::unloadSocketLib();
    if (mConfig) {
        mConfig->drop();
        mConfig = 0;
    }
}


void CNetServerAcceptor::run() {
    SContextIO* iAction = 0;
    const s32 maxe = mConfig->mMaxFetchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    s32 gotsz = 0;

    for (; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, APP_THREAD_MAX_SLEEP_TIME);
        if (gotsz > 0) {
            bool stop = false;
            for (u32 i = 0; i < gotsz; ++i) {
                if (APP_SERVER_EXIT_CODE == iEvent[i].mKey) {
                    stop = true;
                    continue;
                }
                iAction = APP_GET_VALUE_POINTER(iEvent[i].mPointer, SContextIO, mOverlapped);
                iAction->mBytes = iEvent[i].mBytes;
                stepAccpet(mAllWaiter[iAction->mID]);
                //CLogger::log(ELOG_ERROR, "CNetServerAcceptor::run", "unknown operation type");
            }//for
            mRunning = !stop;
        } else {
            s32 pcode = mPoller.getError();
            switch (pcode) {
            case WAIT_TIMEOUT:
                break;

            case ERROR_ABANDONED_WAIT_0: //socket closed
                APP_ASSERT(0);
                break;

            default:
                CLogger::log(ELOG_WARN, "CNetServerAcceptor::run",
                    "invalid overlap, ecode: [%lu]", pcode);
                APP_ASSERT(0);
                break;
            }//switch
            //continue;
        }//else if
    }//while

    delete[] iEvent;
    CLogger::log(ELOG_INFO, "CNetServerAcceptor::run", "acceptor thread exited");
}


bool CNetServerAcceptor::clearError() {
    s32 ecode = CNetSocket::getError();
    switch (ecode) {
    case WSA_IO_PENDING:
        return true;
    case ERROR_SEM_TIMEOUT: //hack? 每秒收到5000个以上的Accept时出现
        return true;
    case WAIT_TIMEOUT: //same as WSA_WAIT_TIMEOUT
        CLogger::log(ELOG_INFO, "CNetServerAcceptor::clearError", "socket timeout, retry...");
        return true;
    case ERROR_CONNECTION_ABORTED: //服务器主动关闭
    case ERROR_NETNAME_DELETED: //客户端主动关闭连接
        return true;
    default:
    {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch
}


bool CNetServerAcceptor::start(CNetConfig* cfg) {
    if (mRunning || nullptr == cfg) {
        CLogger::log(ELOG_INFO, "CNetServerAcceptor::start",
            "server is running already, config=%p", cfg);
        return true;
    }
    APP_ASSERT(cfg);
    cfg->grab();
    mConfig = cfg;
    mAllWaiter.reallocate(cfg->mMaxPostAccept);

    if (!initialize()) {
        removeAll();
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::start", "init server fail");
        return false;
    }

    mRunning = true;
    createServer();

    if (0 == mThread) {
        mThread = new CThread();
    }
    mThread->start(*this);

    CLogger::log(ELOG_INFO, "CNetServerAcceptor::start",
        "success, posted accpet: [%d]", mAcceptCount);
    return true;
}


bool CNetServerAcceptor::stop() {
    if (!mRunning) {
        CLogger::log(ELOG_INFO, "CNetServerAcceptor::stop", "server had stoped.");
        return true;
    }
    CEventPoller::SEvent evt = { 0 };
    evt.mKey = APP_SERVER_EXIT_CODE;
    if (mPoller.postEvent(evt)) {
        /*while(mRunning) {
            CThread::sleep(500);
        }*/
        mThread->join();
        delete mThread;
        mThread = 0;

        removeAll();
        deleteServer();
        APP_LOG(ELOG_INFO, "CNetServerAcceptor::stop", "server stoped success");
        return true;
    }
    return false;
}


void CNetServerAcceptor::removeAll() {
    mListener.close();

    //waiter
    for (u32 i = 0; i < mAllWaiter.size(); ++i) {
        delete mAllWaiter[i];
    }
    mAllWaiter.setUsed(0);
    mAcceptCount = 0;
}


bool CNetServerAcceptor::initialize() {
    if (!mListener.openSeniorTCP()) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "create listen socket fail: %d", CNetSocket::getError());
        return false;
    }

    if (mConfig->mReuse && mListener.setReuseIP(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse IP fail: [%d]", CNetSocket::getError());
        return false;
    }

    if (mConfig->mReuse && mListener.setReusePort(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse port fail: [%d]", CNetSocket::getError());
        return false;
    }

    if (mListener.bind(mAddressLocal)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "bind socket error: [%d]", CNetSocket::getError());
        return false;
    }

    if (mListener.listen(SOMAXCONN)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "listen socket error: [%d]", CNetSocket::getError());
        return false;
    }
    CLogger::log(ELOG_CRITICAL, "CNetServerAcceptor::initialize", "listening: [%s:%d]",
        mAddressLocal.getIPString(), mAddressLocal.getPort());

    mFunctionAccept = mListener.getFunctionAccpet();
    if (!mFunctionAccept) {
        return false;
    }
    mFunctionAcceptSockAddress = mListener.getFunctionAcceptSockAddress();
    if (!mFunctionAcceptSockAddress) {
        return false;
    }

    if (!mPoller.add(mListener, (void*)& mListener)) {
        return false;
    }

    return postAccept();
}


bool CNetServerAcceptor::postAccept() {
    for (u32 id = mAcceptCount; id < mConfig->mMaxPostAccept; ++id) {
        SContextWaiter* waiter = new SContextWaiter();
        waiter->mContextIO->mID = id;
        waiter->mContextIO->mOperationType = EOP_ACCPET;
        if (!postAccept(waiter)) {
            delete waiter;
            CLogger::log(ELOG_ERROR, "CNetServerAcceptor::postAccept", "post accpet fail, id: [%d]", id);
            return false;
        }
        mAllWaiter.pushBack(waiter);
    }
    return true;
}


bool CNetServerAcceptor::postAccept(SContextWaiter* iContext) {
    if (!iContext->reset()) {
        return false;
    }
    bool ret = mListener.accept(iContext->mSocket, iContext->mContextIO, iContext->mCache, mFunctionAccept);
    if (!ret) {
        return false;
    }
    ++mAcceptCount;
    return ret;
}


bool CNetServerAcceptor::stepAccpet(SContextWaiter* iContext) {
    --mAcceptCount;
    if (mReceiver && mService) {
        INetEventer* evt = mReceiver->onAccept(mAddressLocal);
        if (evt) {
            mListener.getAddress(iContext->mCache, mAddressLocal, mAddressRemote, mFunctionAcceptSockAddress);
            u32 nid = mService->receive(iContext->mSocket, mAddressRemote, mAddressLocal, evt);
            if (0 == nid) {
                CLogger::log(ELOG_ERROR, "CNetServerAcceptor::stepAccpet",
                    "[can't add in service, socket:%s:%u->%s:%u]",
                    mAddressRemote.getIPString(), mAddressRemote.getPort(),
                    mAddressLocal.getIPString(), mAddressLocal.getPort());
                iContext->mSocket.close();
            }
        } else {
            CLogger::log(ELOG_ERROR, "CNetServerAcceptor::stepAccpet",
                "[INetEventer::onAccept fail, socket:%s:%u->%s:%u]",
                mAddressRemote.getIPString(), mAddressRemote.getPort(),
                mAddressLocal.getIPString(), mAddressLocal.getPort());
            iContext->mSocket.close();
        }
    } else {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::stepAccpet",
            "[none INetEventer/Service, socket:%s:%u->%s:%u]",
            mAddressRemote.getIPString(), mAddressRemote.getPort(),
            mAddressLocal.getIPString(), mAddressLocal.getPort());
        iContext->mSocket.close();
    }//if

    return postAccept(iContext);
}


CNetServiceTCP* CNetServerAcceptor::createServer() {
    if (mService) {
        return mService;
    }

    CNetServiceTCP* server = new CNetServiceTCP();
    server->setID(0); //@note: step 1
    if (server->start(mConfig)) { //@note: step 2
        mService = server;
        mCreated = true;
        return server;
    }
    delete server;
    return mService;
}


void CNetServerAcceptor::deleteServer() {
    if (mService && mCreated) {
        mService->stop();
        delete mService;
    }
    mService = nullptr;
}


s32 CNetServerAcceptor::send(u32 id, const void* buffer, s32 size) {
    return mService->send(id, buffer, size);
}


CNetServiceTCP* CNetServerAcceptor::getServer() const {
    return mService;
}


void CNetServerAcceptor::setServer(CNetServiceTCP* it) {
    mService = it;
}



}//namespace net
}//namespace app

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace app {
namespace net {

CNetServerAcceptor::CNetServerAcceptor() :
    mReceiver(0),
    mCurrent(0),
    mAcceptCount(0),
    mThread(0),
    mConfig(nullptr),
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetServerAcceptor::~CNetServerAcceptor() {
    stop();
    if (mConfig) {
        mConfig->drop();
        mConfig = nullptr;
    }
    CNetUtility::unloadSocketLib();
}


void CNetServerAcceptor::run() {
    const s32 maxe = mConfig->mMaxFetchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    s32 gotsz = 0;
    CNetSocket sock(mPoller.getSocketPair().getSocketA());
    for (; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, APP_THREAD_MAX_SLEEP_TIME);
        if (gotsz > 0) {
            //bool ret = true;
            for (u32 i = 0; i < gotsz; ++i) {
                if (EPOLLIN & iEvent[i].mEvent) {
                    if (mListener.getValue() == iEvent[i].mData.mData32) {
                        CNetSocket sock(mListener.accept());
                        stepAccpet(sock);
                    } else {
                        u32 mask;
                        s32 ret = sock.receiveAll(&mask, sizeof(mask));
                        if (ENET_SESSION_MASK == mask) {
                            mRunning = false;
                        }
                    }
                } else {
                    CLogger::log(ELOG_ERROR, "CNetServerAcceptor::run", "unknown operation type");
                    continue;
                }
            }//for
        } else if (0 == gotsz) {
            APP_LOG(ELOG_DEBUG, "CNetServerAcceptor::run", "epoll wait timeout");
        } else {
            s32 pcode = mPoller.getError();
            CLogger::log(ELOG_ERROR, "CNetServerAcceptor::run", "epoll wait ecode=[%d]", pcode);
            continue;
        }
    }//for

    delete[] iEvent;
    CLogger::log(ELOG_INFO, "CNetServerAcceptor::run", "acceptor thread exited");
}


bool CNetServerAcceptor::clearError() {
    s32 ecode = CNetSocket::getError();
    switch (ecode) {
    default:
    {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch
}


bool CNetServerAcceptor::start(CNetConfig* cfg) {
    if (mRunning || nullptr == cfg) {
        CLogger::log(ELOG_INFO, "CNetServerAcceptor::start",
            "server is running already, config=%p", cfg);
        return true;
    }
    APP_ASSERT(cfg);
    cfg->grab();
    mConfig = cfg;

    if (!initialize()) {
        removeAll();
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::start", "init server fail");
        return false;
    }

    mRunning = true;
    createServer();

    if (0 == mThread) {
        mThread = new CThread();
    }
    mThread->start(*this);

    CLogger::log(ELOG_INFO, "CNetServerAcceptor::start",
        "success, posted accpet: [%d]", mAcceptCount);
    return true;
}


bool CNetServerAcceptor::stop() {
    if (!mRunning) {
        //CLogger::log(ELOG_INFO, "CNetServerAcceptor::stop", "server had stoped.");
        return true;
    }
    mRunning = false;
    CEventPoller::SEvent evt;
    evt.setMessage(APP_SERVER_EXIT_CODE);
    if (mPoller.postEvent(evt)) {
        mThread->join();
        delete mThread;
        mThread = 0;

        removeAll();
        removeAllServer();
        //APP_LOG(ELOG_INFO, "CNetServerAcceptor::stop", "server stoped success");
        return true;
    }
    return false;
}


s32 CNetServerAcceptor::send(u32 id, const void* buffer, s32 size) {
    u32 sid = ((id & ENET_SERVER_MASK) >> ENET_SESSION_BITS);
    if (sid < mService.size()) {
        return mService[sid]->send(id, buffer, size);
    }
    return -1;
}

void CNetServerAcceptor::removeAll() {
    mListener.close();
    mAcceptCount = 0;
}


bool CNetServerAcceptor::initialize() {
    if (!mListener.openTCP()) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "create listen socket fail: %d", CNetSocket::getError());
        return false;
    }

    if (mConfig->mReuse && mListener.setReuseIP(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse IP fail: [%d]", CNetSocket::getError());
        return false;
    }

    if (mConfig->mReuse && mListener.setReusePort(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse port fail: [%d]", CNetSocket::getError());
        return false;
    }

    if (mListener.bind(mAddressLocal)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "bind socket error: [%d]", CNetSocket::getError());
        return false;
    }

    if (mListener.listen(SOMAXCONN)) {
        CLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "listen socket error: [%d]", CNetSocket::getError());
        return false;
    }

    CNetSocket& sock = mPoller.getSocketPair().getSocketA();
    if (!sock.isOpen()) {
        //printf("error %d on socketpair\n", errno);
        return false;
    }
    CEventPoller::SEvent evt;
    evt.mData.mData32 = ENET_SESSION_MASK;
    evt.mEvent = EPOLLIN | EPOLLERR;
    if (!mPoller.add(sock, evt)) {
        return false;
    }

    //CEventPoller::SEvent evt = {0};
    evt.mData.mData32 = mListener.getValue();
    return mPoller.add(mListener, evt);
}


bool CNetServerAcceptor::stepAccpet(CNetSocket& sock) {
    INetEventer* evt = mReceiver->onAccept(mAddressLocal);
    --mAcceptCount;
    CNetServiceTCP* server = 0;
    u32 sz = mService.size();
    if (0 == sz) {
        createServer();
        sz = mService.size();
        APP_ASSERT(sz > 0);
    }
    sock.getLocalAddress(mAddressLocal);
    sock.getRemoteAddress(mAddressRemote);
    if (mCurrent >= sz) {
        mCurrent = 0;
    }
    do {
        server = mService[mCurrent++];
        server->receive(sock, mAddressRemote, mAddressLocal, mReceiver);
    } while (!server && mCurrent < sz);

    if (!server) {
        server = createServer();
        APP_ASSERT(server);
        server->receive(sock, mAddressRemote, mAddressLocal, mReceiver);
    }

    return 0 != server;
}


CNetServiceTCP* CNetServerAcceptor::createServer() {
    CNetServiceTCP* server = new CNetServiceTCP();
    if (server->start(mConfig)) {
        addServer(server);
        return server;
    }
    delete server;
    return 0;
}


void CNetServerAcceptor::removeAllServer() {
    u32 sz = mService.size();
    for (u32 i = 0; i < sz; ) {
        if (!mService[i]->stop()) {
            CThread::sleep(100);
        } else {
            ++i;
        }
    }
}


void CNetServerAcceptor::addServer(CNetServiceTCP* it) {
    mService.pushBack(it);
}


bool CNetServerAcceptor::removeServer(CNetServiceTCP* it) {
    u32 sz = mService.size();
    for (u32 i = 0; i < sz; ++i) {
        if (it == mService[i]) {
            mService[i] = mService[sz - 1];
            mService.set_used(sz - 1);
            return true;
        }
    }
    return false;
}


}//namespace net
}//namespace app

#endif //APP_PLATFORM_LINUX
