#include "CNetServerAcceptor.h"
#include "CNetUtility.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "SClientContext.h"


//Max accept
#define APP_MAX_POST_ACCEPT 10
///The infomation to inform workers quit.
#define APP_SERVER_EXIT_CODE  0
///cache count for per client context
#define APP_DEFAULT_CLIENT_CACHE (10)

#define APP_THREAD_MAX_SLEEP_TIME  0xffffffff


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {


CNetServerAcceptor::SContextWaiter::SContextWaiter() {
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
    mCurrent(0),
    mAcceptCount(0),
    mThread(0),
    mAllWaiter(APP_MAX_POST_ACCEPT),
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetServerAcceptor::~CNetServerAcceptor() {
    stop();
    CNetUtility::unloadSocketLib();
}


void CNetServerAcceptor::run() {
    SContextIO* iAction = 0;
    const u32 maxe = 20;
    CEventPoller::SEvent iEvent[maxe];
    u32 gotsz = 0;

    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, APP_THREAD_MAX_SLEEP_TIME);
        if(gotsz > 0) {
            bool ret = true;
            for(u32 i = 0; i < gotsz; ++i) {
                if(APP_SERVER_EXIT_CODE == iEvent[i].mKey) {
                    //mRunning = false;
                    continue;
                }
                ret = true;
                iAction = APP_GET_VALUE_POINTER(iEvent[i].mPointer, SContextIO, mOverlapped);
                iAction->mBytes = iEvent[i].mBytes;
                ret = stepAccpet(mAllWaiter[iAction->mID]);
                IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::run", "unknown operation type");
            }//for

        } else {
            s32 pcode = mPoller.getError();
            switch(pcode) {
            case WAIT_TIMEOUT:
                break;

            case ERROR_ABANDONED_WAIT_0: //socket closed
                APP_ASSERT(0);
                break;

            default:
                IAppLogger::log(ELOG_WARNING, "CNetClientSeniorTCP::run",
                    "invalid overlap, ecode: [%lu]", pcode);
                APP_ASSERT(0);
                break;
            }//switch
            continue;
        }//else if
    }//while
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
        mRunning = false;
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
    for(u32 id = mAcceptCount; id < APP_MAX_POST_ACCEPT; ++id) {
        SContextWaiter* waiter = new SContextWaiter();
        waiter->mContextIO->mID = id;
        waiter->mContextIO->mOperationType = EOP_ACCPET;
        if(!postAccept(waiter)) {
            delete waiter;
            IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "post accpet fail, id: [%d]", id);
            return false;
        }
        mAllWaiter.push_back(waiter);
        ++mAcceptCount;
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
        server->addSession(iContext->mSocket, mAddressRemote, mAddressLocal);
    } while(!server && mCurrent < sz);

    if(!server) {
        server = createServer();
        APP_ASSERT(server);
        server->addSession(iContext->mSocket, mAddressRemote, mAddressLocal);
    }

    return postAccept(iContext);
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

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace irr {
namespace net {
CNetServerAcceptor::CNetServerAcceptor() : mThreadCount(APP_NET_DEFAULT_SERVER_THREAD),
mMaxClientCount(0x7FFFFFFF),
mRunning(false),
mReceiver(0),
mAddressLocal(APP_NET_ANY_IP, APP_NET_DEFAULT_PORT) {

}


CNetServerAcceptor::~CNetServerAcceptor() {
    stop();
}


bool CNetServerAcceptor::initialize() {
    if(!mListener.openTCP()) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "open socket fail: [%d]", CNetSocket::getError());
        return false;
    }

    if(mListener.setBlock(false)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "set no block fail: [%d]", CNetSocket::getError());
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

    if(!mListener.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "bind local fail: [%d]", CNetSocket::getError());
        return false;
    }

    if(!mListener.listen(mMaxClientCount)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "listen fail: [%d]", CNetSocket::getError());
        return false;
    }

    mAccpetIO = new SContextIO();
    mAccpetIO->mOperationType = EOP_ACCPET;

    if(!postAccept(mAccpetIO)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerAcceptor::initialize", "post accept fail: %d", errno);
    }


    //user should according to cpu cores to create worker threads
    mAllWorker.reallocate(mThreadCount, false);
    CThread* wk;
    for(u32 i = 0; i < mThreadCount; ++i) {
        wk = new CThread();
        wk->start(*this);
        mAllWorker.push_back(wk);
        CThread::sleep(20);
    }

    IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::initialize", "created worker thread, total: %d", mThreadCount);
    return true;
}


void CNetServerAcceptor::run() {
    IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::run", "worker theread start, ID: %d", CThread::getCurrentNativeID());
    CEventPoller::SEvent allEvent[APP_MAX_POST_ACCEPT];
    CNetSession* iContextSocket = 0;
    s32 ret;
    for(; mRunning;) {
        ret = mEpollFD.getEvents(allEvent, APP_MAX_POST_ACCEPT, 100);
        if(ret >= 0) {
            for(s32 n = 0; n < ret; ++n) {
                iContextSocket = (CNetSession*) allEvent[n].mData.mPointer;
                switch(allEvent[n].mEvent) {
                case EOP_RECEIVE:
                    stepReceive(iContextSocket);
                    break;
                case EOP_SEND:
                    stepSend(iContextSocket);
                    break;
                case EOP_ACCPET:
                    //stepAccpet(iContextSocket);
                    break;
                }//switch
            }//for
        } else {
            //error
        }
    }//for

    IAppLogger::log(ELOG_INFO, "CNetServerAcceptor::run", "worker thread exit, ID: %d", CThread::getCurrentNativeID());
}



}// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_LINUX
