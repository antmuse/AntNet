#include "CNetServerSeniorUDP.h"
#include "CNetUtility.h"
#include "CTimer.h"
#include "CLogger.h"
#include "CNetPacket.h"


#define APP_SERVER_OVERTIME  15*1000


namespace app {
namespace net {

CNetServerSeniorUDP::CNetServerSeniorUDP() :
    mAddressLocal(APP_NET_DEFAULT_PORT),
    mMaxClientCount(0xFFFFFFFF),
    mRunning(false),
    mCurrentTime(0),
    mOverTimeInterval(APP_SERVER_OVERTIME),
    mReceiver(0) {
    CNetUtility::loadSocketLib();
}


CNetServerSeniorUDP::~CNetServerSeniorUDP() {
    stop();
    CNetUtility::unloadSocketLib();
}


void CNetServerSeniorUDP::run() {
    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::run", "worker theread start, ID: %d", CThread::getCurrentNativeID());
    s8* buffer = new s8[APP_NET_MAX_BUFFER_LEN]; //buffer for each thread
    s32 ret = 0;
    u64 last_tick = CTimer::getRelativeTime();

    for(; mRunning;) {
        mCurrentTime = CTimer::getRelativeTime();
        ret = mConnector.receiveFrom(buffer, APP_NET_MAX_BUFFER_LEN, mAddressRemote);
        if(APP_SOCKET_ERROR == ret) {
            if(!clearError()) {
                break;
            }
        } else {
            CNetAddress::ID cid = mAddressRemote.getID();
            core::TMap<CNetAddress::ID, SClientContextUDP*>::Node* node = mAllClient.find(cid);
            if(node) {
                SClientContextUDP* client = node->getValue();
                client->mProtocal.import(buffer, ret);
                if(mCurrentTime >= client->mNextTime) {
                    client->mProtocal.update(mCurrentTime);
                    client->mNextTime = client->mProtocal.check(mCurrentTime);
                }
                stepReceive(client);
                client->mOverTime = mCurrentTime + mOverTimeInterval;
            } else { //new client
                SClientContextUDP* newContext = new SClientContextUDP();
                mAddressRemote.reverse();
                newContext->mClientAddress = mAddressRemote;
                addClient(newContext);
                newContext->mProtocal.import(buffer, ret);
                stepReceive(newContext);
            }
        }

        //update others
        for(core::TMap<CNetAddress::ID, SClientContextUDP*>::Iterator it = mAllClient.getIterator();
            !it.atEnd(); it++) {
            SClientContextUDP* client = it->getValue();
            if(mCurrentTime >= client->mNextTime) {
                client->mProtocal.update(mCurrentTime);
                client->mNextTime = client->mProtocal.check(mCurrentTime);
            }
        }

        if(mCurrentTime > (mOverTimeInterval + last_tick)) {
            removeOvertimeClient();
            last_tick += mOverTimeInterval;
            //CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::run", "tick time: [%u]", last_tick);
        }

        CThread::sleep(20);
    }//for

     //mRunning = false;
    delete[] buffer;

    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::run", "worker thread exit, ID: %d", CThread::getCurrentNativeID());
}


void CNetServerSeniorUDP::removeOvertimeClient() {
    //CAutoLock aulock(mMutex);
    core::TMap<CNetAddress::ID, SClientContextUDP*>::Node* node;
    SClientContextUDP* context;
    u32 count = 0;
    for(core::TMap<CNetAddress::ID, SClientContextUDP*>::Iterator it = mAllClient.getIterator();
        !it.atEnd(); ) {
        node = it.getNode();
        context = node->getValue();
        if(context->mOverTime < mCurrentTime) {
            CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::removeOvertimeClient",
                "remove peer time:[%u],thread ID:[%d]",
                context->mOverTime,
                CThread::getCurrentNativeID());

            delete context;
            it++;
            count++;
            mAllClient.remove(node);
        } else {
            it++;
        }
    }

    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::removeOvertimeClient",
        "removed:[%u], hold client:[%u], time:[%u/s], thread ID:[%d]",
        count,
        mAllClient.size(),
        mCurrentTime / 1000,
        CThread::getCurrentNativeID());
}


bool CNetServerSeniorUDP::start() {
    if(mRunning) {
        CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::start", "server is running currently.");
        return false;
    }


    if(false == initialize()) {
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::start", "init server fail");
        removeAll();
        return false;
    }

    mThread = new CThread();
    mThread->start(*this);
    mRunning = true;

    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::start", "server start success");
    return true;
}


bool CNetServerSeniorUDP::stop() {
    if(!mRunning) {
        return false;
    }

    mRunning = false;
    mThread->join();
    removeAllClient();
    removeAll();
    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::stop", "server stoped success");
    return true;
}


bool CNetServerSeniorUDP::initialize() {
    if(!mConnector.openUDP()) {
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "create listen socket fail: %d", mConnector.getError());
        return false;
    }

    if(mConnector.setBlock(false)) {
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "set socket unblocked fail");
    }

    if(mConnector.bind(mAddressLocal)) {
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "bind socket error: %d", mConnector.getError());
        return false;
    }
    mConnector.getLocalAddress(mAddressLocal);
    mAddressLocal.reverse();

    if(mConnector.setReuseIP(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "socket reuse IP error: %d", mConnector.getError());
        return false;
    }

    if(mConnector.setReusePort(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "socket reuse port error: %d", mConnector.getError());
        return false;
    }

    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::initialize", "init ok");
    return true;
}


void CNetServerSeniorUDP::removeAll() {
    delete mThread;
    mConnector.close();
    mThread = 0;
    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::removeAll", "remove all success");
}


bool CNetServerSeniorUDP::stepReceive(SClientContextUDP* iContext) {
    net::CNetPacket& pack = iContext->mNetPack;
    s32 ret = iContext->mProtocal.receiveData(pack.getPointer(), pack.getAllocatedSize());
    if(ret > 0) {
        pack.setUsed(ret);
        onPacket(pack);
        iContext->mProtocal.sendData("got", 4);
    }//if
    return true;
}


void CNetServerSeniorUDP::addClient(SClientContextUDP* iContext) {
    CNetAddress::ID cid = iContext->mClientAddress.getID();

    mAllClient.set(cid, iContext);
    //iContext->mProtocal.setID(0);
    iContext->mProtocal.setUserPointer(iContext);
    iContext->mProtocal.setSender(this);

    CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::addClient",
        "incoming client [%d=%s:%d]",
        getClientCount(),
        iContext->mClientAddress.getIPString(),
        iContext->mClientAddress.getPort());
}


void CNetServerSeniorUDP::removeClient(CNetAddress::ID id) {
    core::TMap<CNetAddress::ID, SClientContextUDP*>::Node* node = mAllClient.delink(id);
    if(node) {
        SClientContextUDP* iContextSocket = node->getValue();
        delete iContextSocket;
        delete node;
        CLogger::log(ELOG_INFO, "CNetServerSeniorUDP::removeClient", "client count: %u", getClientCount());
    }
}


void CNetServerSeniorUDP::removeAllClient() {
    for(core::TMap<CNetAddress::ID, SClientContextUDP*>::Iterator it = mAllClient.getIterator();
        !it.atEnd(); it++) {
        delete it->getValue();
    }
    mAllClient.clear();
}


bool CNetServerSeniorUDP::clearError() {
    s32 ecode = mConnector.getError();

    switch(ecode) {
#if defined(APP_PLATFORM_WINDOWS)
    case 0:
    case WSAEWOULDBLOCK: //none data
        return true;
    case WSAECONNRESET: //reset
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    case 0:
        //case EAGAIN:
    case EWOULDBLOCK: //none data
        return true;
    case ECONNRESET: //reset
#endif
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::clearError", "remote reseted: %d", ecode);
        return false;
    default:
        CLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::clearError", "socket error: %d", ecode);
        return false;
    }//switch

    return false;
}


s32 CNetServerSeniorUDP::sendBuffer(void* iUserPointer, const s8* iData, s32 iLength) {
    if(!iUserPointer) return -1;

    SClientContextUDP* iContext = (SClientContextUDP*) iUserPointer;
    s32 ret = 0;
    s32 sent;
    for(; ret < iLength;) {
        sent = mConnector.sendto(iData + ret, iLength - ret, iContext->mClientAddress);
        if(sent > 0) {
            ret += sent;
        } else {
            if(!clearError()) {
                break;
            }
            CThread::yield();
        }
    }
    APP_LOG(ELOG_INFO, "CNetServerSeniorUDP::sendBuffer", "Sent leftover: [%d - %d]", iLength, ret);
    return ret;
}


APP_INLINE void CNetServerSeniorUDP::onPacket(CNetPacket& it) {
    if(mReceiver) {
        mReceiver->onReceive(mAddressRemote, 0, it.getReadPointer(), it.getReadSize());
    }
}


}//namespace net
}//namespace app
