#include "CNetServerNatPuncher.h"
#include "CThread.h"
#include "CNetUtility.h"
#include "CNetPacket.h"
#include "CTimer.h"
#include "CLogger.h"

//TODO>> fix

#define APP_SERVER_OVERTIME  15*1000
#define APP_SERVER_PORT  9981

namespace app {
namespace net {

CNetServerNatPuncher::CNetServerNatPuncher() :
    mCurrentTime(0),
    mThread(0),
    mRunning(false),
    mOverTimeInterval(APP_SERVER_OVERTIME),
    mAddressLocal(APP_SERVER_PORT) {
    CNetUtility::loadSocketLib();
}


CNetServerNatPuncher::~CNetServerNatPuncher() {
    CNetUtility::unloadSocketLib();
}


bool CNetServerNatPuncher::initialize() {
    if(!mConnector.openUDP()) {
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::initialize", "create listen socket fail: %d", mConnector.getError());
        return false;
    }

    if(mConnector.setBlock(false)) {
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::initialize", "set socket unblocked fail");
    }

    if(mConnector.bind(mAddressLocal)) {
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::initialize", "bind socket error: %d", mConnector.getError());
        return false;
    }
    mConnector.getLocalAddress(mAddressLocal);
    mAddressLocal.reverse();

    if(mConnector.setReuseIP(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::initialize", "socket reuse IP error: %d", mConnector.getError());
        return false;
    }

    if(mConnector.setReusePort(true)) {
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::initialize", "socket reuse port error: %d", mConnector.getError());
        return false;
    }

    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::initialize", "success on [%s:%u]",
        mAddressLocal.getIPString(),
        mAddressLocal.getPort());

    return true;
}


bool CNetServerNatPuncher::start() {
    if(mRunning) {
        CLogger::log(ELOG_INFO, "CNetServerNatPuncher::start",
            "server is running already.");
        return false;
    }


    if(false == initialize()) {
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::start", "init server fail");
        removeAll();
        return false;
    }

    mRunning = true;
    mThread = new CThread();
    mThread->start(*this);

    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::start", "server start success");
    return true;
}


bool CNetServerNatPuncher::stop() {
    if(!mRunning) {
        return false;
    }

    mRunning = false;
    mThread->join();
    //removeAllClient();
    removeAll();
    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::stop", "server stoped success");
    return true;
}


bool CNetServerNatPuncher::clearError() {
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
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::clearError", "remote reseted: %d", ecode);
        return false;
    default:
        CLogger::log(ELOG_ERROR, "CNetServerNatPuncher::clearError", "socket error: %d", ecode);
        return false;
    }//switch

    return false;
}


const core::TMap<CNetAddress::ID, CNetServerNatPuncher::SClientNode*>::Node*
CNetServerNatPuncher::getRemote(CNetAddress::ID& id)const {
    for(core::TMap<CNetAddress::ID, SClientNode*>::ConstIterator it = mAllClient.getConstIterator();
        !it.atEnd(); it++) {
        if(it->getKey() == id) {
            return it.getNode();
        }
    }
    return 0;
}



const core::TMap<CNetAddress::ID, CNetServerNatPuncher::SClientNode*>::Node*
CNetServerNatPuncher::getAnyRemote(CNetAddress::ID& my)const {
    for(core::TMap<CNetAddress::ID, SClientNode*>::ConstIterator it = mAllClient.getConstIterator();
        !it.atEnd(); it++) {
        if(it->getKey() != my) {
            return it.getNode();
        }
    }
    return 0;
}


void CNetServerNatPuncher::run() {
    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::run", "worker theread start, ID: %d", CThread::getCurrentNativeID());
    CNetPacket pack(1024);
    s32 ret = 0;
    u64 last_tick = CTimer::getRelativeTime();

    for(; mRunning;) {
        mCurrentTime = CTimer::getRelativeTime();
        //pack.setUsed(0);
        ret = mConnector.receiveFrom(pack.getPointer(), pack.getAllocatedSize(), mAddressRemote);
        if(ret > 0) {
            mAddressRemote.reverse();
            CNetAddress::ID cid = mAddressRemote.getID();
            core::TMap<CNetAddress::ID, SClientNode*>::Node* node = mAllClient.find(cid);
            SClientNode* client = node ? node->getValue() : 0;

            pack.setUsed(ret);
            switch(pack.readU8()) {
            case ENET_TIMEOUT:
            {
                if(!client) {
                    client = new SClientNode();
                    client->mAddress = mAddressRemote;
                    mAllClient.insert(cid, client);
                    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::run",
                        "new client: [%s:%u]",
                        mAddressRemote.getIPString(),
                        mAddressRemote.getPort());
                }
                client->mTime = mCurrentTime + mOverTimeInterval;
                pack.setUsed(0);
                pack.add(u8(ENET_TIMEOUT));
                pack.add(cid);
                mConnector.sendto(pack.getPointer(), pack.getSize(), mAddressRemote);
                break;
            }
            case ENET_SEND:
            {
                CNetAddress::ID peerid = pack.readU64();
                const core::TMap<CNetAddress::ID, SClientNode*>::Node* peer = 0;
                if(0 == peerid) {
                    peer = getAnyRemote(peerid);
                } else {
                    peer = getRemote(peerid);
                }
                if(peer) {
                    pack.setUsed(0);
                    pack.add(u8(ENET_SEND));
                    const CNetAddress::ID& pid = peer->getKey();
                    pack.add(pid);
                    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::run",
                        "peer a: [%u]->[%s:%u]", pid,
                        mAddressRemote.getIPString(), mAddressRemote.getPort());

                    u32 psize = pack.getSize();

                    pack.add(u8(ENET_SEND));
                    const CNetAddress& pb = peer->getValue()->mAddress;
                    pack.add(pb.getID());

                    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::run",
                        "peer b: [%u]->[%s:%u]", pb.getID(),
                        pb.getIPString(), pb.getPort());

                    for(u32 i = 0; i < 5; i++) {
                        mConnector.sendto(pack.getPointer(), psize, mAddressRemote);
                        mConnector.sendto(pack.getPointer() + psize, psize, pb);
                    }
                }
                break;
            }
            case ENET_DISCONNECT:
            {
                if(client) {
                    delete client;
                    mAllClient.remove(node);
                }
                break;
            }
            }//switch
        } else if(0 == ret) {

        } else {
            if(!clearError()) {
                break;
            }
        }

        if(mCurrentTime > last_tick) {
            checkTimeout();
            last_tick += mOverTimeInterval;
        }
        CThread::sleep(20);
    }//for

    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::run", "worker thread exit, ID: %d", CThread::getCurrentNativeID());
}


void CNetServerNatPuncher::checkTimeout() {
    //CAutoLock aulock(mMutex);
    core::TMap<CNetAddress::ID, SClientNode*>::Node* node;
    SClientNode* context;
    u32 count = 0;
    for(core::TMap<CNetAddress::ID, SClientNode*>::Iterator it = mAllClient.getIterator();
        !it.atEnd(); ) {
        node = it.getNode();
        context = node->getValue();
        if(context->mTime < mCurrentTime) {
            CLogger::log(ELOG_INFO, "CNetServerNatPuncher::checkTimeout",
                "remove peer time:[%u],thread ID:[%d]",
                context->mTime,
                CThread::getCurrentNativeID());

            delete context;
            it++;
            count++;
            mAllClient.remove(node);
        } else {
            it++;
        }
    }

    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::checkTimeout",
        "removed:[%u], hold client:[%u], time:[%u/s], thread ID:[%d]",
        count,
        mAllClient.size(),
        mCurrentTime / 1000,
        CThread::getCurrentNativeID());
}


void CNetServerNatPuncher::removeAllClient() {
    for(core::TMap<CNetAddress::ID, SClientNode*>::Iterator it = mAllClient.getIterator();
        !it.atEnd(); it++) {
        delete it->getValue();
    }
    mAllClient.clear();
}


void CNetServerNatPuncher::removeAll() {
    delete mThread;
    mConnector.close();
    mThread = 0;
    CLogger::log(ELOG_INFO, "CNetServerNatPuncher::removeAll", "remove all success");
}



} //namespace net
} //namespace app
