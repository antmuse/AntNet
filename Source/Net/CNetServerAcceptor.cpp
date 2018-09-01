#include "CNetServerAcceptor.h"
#include "CNetUtility.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "SClientContext.h"


///The infomation to inform workers quit.
#define APP_SERVER_EXIT_CODE  0

#define APP_THREAD_MAX_SLEEP_TIME  0xffffffff


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
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
CNetServerAcceptor::CNetServerAcceptor(CNetConfig* cfg) :
    mReceiver(0),
    mCurrent(0),
    mAcceptCount(0),
    mThread(0),
    mAllWaiter(cfg->mMaxPostAccept),
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mRunning(false) {
    APP_ASSERT(cfg);
    cfg->grab();
    mConfig = cfg;
    CNetUtility::loadSocketLib();
}


CNetServerAcceptor::~CNetServerAcceptor() {
    stop();
    CNetUtility::unloadSocketLib();
    if(mConfig) {
        mConfig->drop();
        mConfig = 0;
    }
}


void CNetServerAcceptor::run() {
    SContextIO* iAction = 0;
    const u32 maxe = mConfig->mMaxFatchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    u32 gotsz = 0;

    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, APP_THREAD_MAX_SLEEP_TIME);
        if(gotsz > 0) {
            bool stop = false;
            for(u32 i = 0; i < gotsz; ++i) {
                if(APP_SERVER_EXIT_CODE == iEvent[i].mKey) {
                    stop = true;
                    continue;
                }
                iAction = APP_GET_VALUE_POINTER(iEvent[i].mPointer, SContextIO, mOverlapped);
                iAction->mBytes = iEvent[i].mBytes;
                stepAccpet(mAllWaiter[iAction->mID]);
                //IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::run", "unknown operation type");
            }//for
            mRunning = !stop;
        } else {
            s32 pcode = mPoller.getError();
            switch(pcode) {
            case WAIT_TIMEOUT:
                break;

            case ERROR_ABANDONED_WAIT_0: //socket closed
                APP_ASSERT(0);
                break;

            default:
                IAppLogger::log(ELOG_WARNING, "CNetServerAcceptor::run",
                    "invalid overlap, ecode: [%lu]", pcode);
                APP_ASSERT(0);
                break;
            }//switch
            //continue;
        }//else if
    }//while

    delete[] iEvent;
    IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::run", "acceptor thread exited");
}


bool CNetServerAcceptor::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    case WSA_IO_PENDING:
        return true;
    case ERROR_SEM_TIMEOUT: //hack? 每秒收到5000个以上的Accept时出现
        return true;
    case WAIT_TIMEOUT: //same as WSA_WAIT_TIMEOUT
        IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::clearError", "socket timeout, retry...");
        return true;
    case ERROR_CONNECTION_ABORTED: //服务器主动关闭
    case ERROR_NETNAME_DELETED: //客户端主动关闭连接
        return true;
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch
}


bool CNetServerAcceptor::start() {
    if(mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::start", "server is running already");
        return true;
    }

    if(!initialize()) {
        removeAll();
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::start", "init server fail");
        return false;
    }

    mRunning = true;
    createServer();

    if(0 == mThread) {
        mThread = new CThread();
    }
    mThread->start(*this);

    IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::start",
        "success, posted accpet: [%d]", mAcceptCount);
    return true;
}


bool CNetServerAcceptor::stop() {
    if(!mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::stop", "server had stoped.");
        return true;
    }
    CEventPoller::SEvent evt = {0};
    evt.mKey = APP_SERVER_EXIT_CODE;
    if(mPoller.postEvent(evt)) {
        /*while(mRunning) {
            CThread::sleep(500);
        }*/
        mThread->join();
        delete mThread;
        mThread = 0;

        removeAll();
        removeAllServer();
        APP_LOG(ELOG_INFO, "CNetServerAcceptor::stop", "server stoped success");
        return true;
    }
    return false;
}


void CNetServerAcceptor::removeAll() {
    mListener.close();

    //waiter
    for(u32 i = 0; i < mAllWaiter.size(); ++i) {
        delete mAllWaiter[i];
    }
    mAllWaiter.set_used(0);
    mAcceptCount = 0;
}


bool CNetServerAcceptor::initialize() {
    if(!mListener.openSeniorTCP()) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "create listen socket fail: %d", CNetSocket::getError());
        return false;
    }

    if(mConfig->mReuse && mListener.setReuseIP(true)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse IP fail: [%d]", CNetSocket::getError());
        return false;
    }

    if(mConfig->mReuse && mListener.setReusePort(true)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse port fail: [%d]", CNetSocket::getError());
        return false;
    }

    if(mListener.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "bind socket error: [%d]", CNetSocket::getError());
        return false;
    }

    if(mListener.listen(SOMAXCONN)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "listen socket error: [%d]", CNetSocket::getError());
        return false;
    }
    IAppLogger::log(ELOG_CRITICAL, "CNetServerAcceptor::initialize", "listening: [%s:%d]", 
        mAddressLocal.getIPString(), mAddressLocal.getPort());

    mFunctionAccept = mListener.getFunctionAccpet();
    if(!mFunctionAccept) {
        return false;
    }
    mFunctionAcceptSockAddress = mListener.getFunctionAcceptSockAddress();
    if(!mFunctionAcceptSockAddress) {
        return false;
    }

    if(!mPoller.add(mListener, (void*) &mListener)) {
        return false;
    }

    return postAccept();
}


bool CNetServerAcceptor::postAccept() {
    for(u32 id = mAcceptCount; id < mConfig->mMaxPostAccept; ++id) {
        SContextWaiter* waiter = new SContextWaiter();
        waiter->mContextIO->mID = id;
        waiter->mContextIO->mOperationType = EOP_ACCPET;
        if(!postAccept(waiter)) {
            delete waiter;
            IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::postAccept", "post accpet fail, id: [%d]", id);
            return false;
        }
        mAllWaiter.push_back(waiter);
    }
    return true;
}


bool CNetServerAcceptor::postAccept(SContextWaiter* iContext) {
    if(!iContext->reset()) {
        return false;
    }
    bool ret = mListener.accept(iContext->mSocket, iContext->mContextIO, iContext->mCache, mFunctionAccept);
    if(!ret) {
        return false;
    }
    ++mAcceptCount;
    return ret;
}


bool CNetServerAcceptor::stepAccpet(SContextWaiter* iContext) {
    --mAcceptCount;
    CNetServerSeniorTCP* server = 0;
    u32 sz = mAllService.size();
    if(0 == sz) {
        createServer();
        sz = mAllService.size();
        APP_ASSERT(sz > 0);
    }
    mListener.getAddress(iContext->mCache, mAddressLocal, mAddressRemote, mFunctionAcceptSockAddress);

    if(mCurrent >= sz) {
        mCurrent = 0;
    }
    do {
        server = mAllService[mCurrent++];
        server->addSession(iContext->mSocket, mAddressRemote, mAddressLocal, mReceiver);
    } while(!server && mCurrent < sz);

    if(!server) {
        server = createServer();
        APP_ASSERT(server);
        server->addSession(iContext->mSocket, mAddressRemote, mAddressLocal, mReceiver);
    }
    return postAccept(iContext);
}


CNetServerSeniorTCP* CNetServerAcceptor::createServer() {
    CNetServerSeniorTCP* server = new CNetServerSeniorTCP(mConfig);
    if(server->start()) {
        addServer(server);
        return server;
    }
    delete server;
    return 0;
}


void CNetServerAcceptor::removeAllServer() {
    u32 sz = mAllService.size();
    for(u32 i = 0; i < sz; ) {
        if(!mAllService[i]->stop()) {
            CThread::sleep(1000);
        } else {
            ++i;
        }
    }
}


void CNetServerAcceptor::addServer(CNetServerSeniorTCP* it) {
    mAllService.push_back(it);
}


bool CNetServerAcceptor::removeServer(CNetServerSeniorTCP* it) {
    u32 sz = mAllService.size();
    for(u32 i = 0; i < sz; ++i) {
        if(it == mAllService[i]) {
            mAllService[i] = mAllService[sz - 1];
            mAllService.set_used(sz - 1);
            return true;
        }
    }
    return false;
}


}//namespace net
}//namespace irr

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace irr {
namespace net {

CNetServerAcceptor::CNetServerAcceptor() :
    mCurrent(0),
    mAcceptCount(0),
    mThread(0),
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetServerAcceptor::~CNetServerAcceptor() {
    stop();
    CNetUtility::unloadSocketLib();
}


void CNetServerAcceptor::run() {
    const u32 maxe = 20;
    CEventPoller::SEvent iEvent[maxe];
    u32 gotsz = 0;

    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, APP_THREAD_MAX_SLEEP_TIME);
        if(gotsz > 0) {
            bool ret = true;
            for(u32 i = 0; i < gotsz; ++i) {
                if(APP_SERVER_EXIT_CODE == iEvent[i].mData.mData32) {
                    mRunning = false;
                    continue;
                }
                ret = true;
                if(EPOLLIN & iEvent[i].mEvent) {
                    CNetSocket sock(mListener.accept());
                    ret = stepAccpet(sock);
                } else {
                    IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::run", "unknown operation type");
                    continue;
                }
            }//for
        } else {
            s32 pcode = mPoller.getError();
            IAppLogger::log(ELOG_WARNING, "CNetServerAcceptor::run",
                "invalid overlap, ecode: [%lu]", pcode);
            continue;
        }//else if
    }//for
}


bool CNetServerAcceptor::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch
}


bool CNetServerAcceptor::start() {
    if(mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::start", "server is running already");
        return true;
    }

    if(!initialize()) {
        removeAll();
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::start", "init server fail");
        return false;
    }

    mRunning = true;
    createServer();

    if(0 == mThread) {
        mThread = new CThread();
    }
    mThread->start(*this);

    IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::start",
        "success, posted accpet: [%d]", mAcceptCount);
    return true;
}


bool CNetServerAcceptor::stop() {
    if(!mRunning) {
        //IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::stop", "server had stoped.");
        return true;
    }
    CEventPoller::SEvent evt;
    evt.setMessage(APP_SERVER_EXIT_CODE);
    if(mPoller.postEvent(evt)) {
        mRunning = false;
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


void CNetServerAcceptor::removeAll() {
    mListener.close();
    mAcceptCount = 0;
}


bool CNetServerAcceptor::initialize() {
    if(!mListener.openTCP()) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "create listen socket fail: %d", CNetSocket::getError());
        return false;
    }

    if(mListener.setReuseIP(true)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse IP fail: [%d]", CNetSocket::getError());
        return false;
    }

    if(mListener.setReusePort(true)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set reuse port fail: [%d]", CNetSocket::getError());
        return false;
    }

    if(mListener.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "bind socket error: [%d]", CNetSocket::getError());
        return false;
    }

    if(mListener.listen(SOMAXCONN)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "listen socket error: [%d]", CNetSocket::getError());
        return false;
    }

    CEventPoller::SEvent evt = {0};
    evt.mData.mData32 = mListener.getValue();
    evt.mEvent = EPOLLIN;
    return mPoller.add(mListener, evt);
}


bool CNetServerAcceptor::stepAccpet(CNetSocket& sock) {
    --mAcceptCount;
    CNetServerSeniorTCP* server = 0;
    u32 sz = mAllService.size();
    if(0 == sz) {
        createServer();
        sz = mAllService.size();
        APP_ASSERT(sz > 0);
    }
    sock.getLocalAddress(mAddressLocal);
    sock.getRemoteAddress(mAddressRemote);
    if(mCurrent >= sz) {
        mCurrent = 0;
    }
    do {
        server = mAllService[mCurrent++];
        server->addSession(sock, mAddressRemote, mAddressLocal);
    } while(!server && mCurrent < sz);

    if(!server) {
        server = createServer();
        APP_ASSERT(server);
        server->addSession(sock, mAddressRemote, mAddressLocal);
    }

    return 0 != server;
}


CNetServerSeniorTCP* CNetServerAcceptor::createServer() {
    CNetServerSeniorTCP* server = new CNetServerSeniorTCP();
    if(server->start()) {
        addServer(server);
        return server;
    }
    delete server;
    return 0;
}


void CNetServerAcceptor::removeAllServer() {
    u32 sz = mAllService.size();
    for(u32 i = 0; i < sz; ) {
        if(!mAllService[i]->stop()) {
            CThread::sleep(100);
        } else {
            ++i;
        }
    }
}


void CNetServerAcceptor::addServer(CNetServerSeniorTCP* it) {
    mAllService.push_back(it);
}


bool CNetServerAcceptor::removeServer(CNetServerSeniorTCP* it) {
    u32 sz = mAllService.size();
    for(u32 i = 0; i < sz; ++i) {
        if(it == mAllService[i]) {
            mAllService[i] = mAllService[sz - 1];
            mAllService.set_used(sz - 1);
            return true;
        }
    }
    return false;
}


}//namespace net
}//namespace irr

#endif //APP_PLATFORM_LINUX
