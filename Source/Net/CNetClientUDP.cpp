#include "CNetClientUDP.h"
#include "CNetUtility.h"
#include "CThread.h"
#include "IAppLogger.h"

//TODO>> fix

#define APP_PER_CACHE_SIZE	(1024)

///The tick time, we need keep the net link alive.
#define APP_NET_TICK_TIME  (30*1000)

///We will relink the socket if tick count >= APP_NET_TICK_MAX_COUNT
#define APP_NET_TICK_MAX_COUNT  (4)

namespace irr {
namespace net {

CNetClientUDP::CNetClientUDP() :
    mAddressLocal(APP_NET_ANY_IP, APP_NET_ANY_PORT),
    mStatus(ENET_INVALID),
    mPacket(APP_PER_CACHE_SIZE),
    mUpdateTime(0),
    mReceiver(0),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetClientUDP::~CNetClientUDP() {
    stop();
    CNetUtility::unloadSocketLib();
}


s32 CNetClientUDP::sendBuffer(void* iUserPointer, const c8* iData, s32 iLength) {
    s32 ret = 0;
    s32 sent;
    for(; ret < iLength;) {
        sent = mConnector.sendto(iData + ret, iLength - ret, mAddressRemote);
        if(sent > 0) {
            ret += sent;
        } else {
            if(!clearError()) {
                break;
            }
            CThread::yield();
        }
    }
    APP_LOG(ELOG_INFO, "CNetClientUDP::sendBuffer", "Sent leftover: [%d - %d]", iLength, ret);
    return ret;
}


s32 CNetClientUDP::send(const void* iData, s32 iLength) {
    switch(mStatus) {
    default:
    case ENET_INVALID:
        return 0;
    }
    return mProtocal.sendData((const c8*)iData, iLength);
}


bool CNetClientUDP::clearError() {
    s32 ecode = mConnector.getError();

    switch(ecode) {
#if defined(APP_PLATFORM_WINDOWS)
    case WSAETIMEDOUT:
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
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::clearError", "server reseted: %d", ecode);
        return false;
    default:
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::clearError", "socket error: %d", ecode);
        return false;
    }//switch

    return false;
}


bool CNetClientUDP::update(s64 iTime) {
    u64 steptime = iTime - mUpdateTime;
    mUpdateTime = iTime;

    switch(mStatus) {
    case ENET_RECEIVE:
        step(steptime);
        break;

    case ENET_TIMEOUT:
        sendTick();
        break;

    case ENET_INVALID:
        resetSocket();
        break;

    case ENET_DISCONNECT:
        sendBye();
        mProtocal.flush();
        mRunning = false;
        mConnector.close();
        return false;

    case ENET_ERROR:
    default:
        mStatus = ENET_DISCONNECT;
        CThread::sleep(1000);
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::run", "Default? How can you come here?");
        break;

    }//switch

    return true;
}


bool CNetClientUDP::start(bool useThread) {
    if(mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetClientUDP::start", "client is running currently.");
        return false;
    }
    mRunning = true;
    mTickCount = 0;
    mTickTime = 0;
    mProtocal.setSender(this);
    //mStatus = ENM_RESET;
    return true;
}


bool CNetClientUDP::stop() {
    if(!mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetClientUDP::stop", "client stoped.");
        return false;
    }

    mStatus = ENET_INVALID;
    mRunning = false;
    return true;
}


void CNetClientUDP::setID(u32 id) {
    mProtocal.setID(id);
}


void CNetClientUDP::resetSocket() {
    mTickCount = 0;
    mTickTime = 0;

    mConnector.close();
    if(mConnector.openUDP()) {
        bindLocal();
        mStatus = ENET_RECEIVE;
        mProtocal.flush();
        IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::resetSocket", "reused previous socket");
    }
}


void CNetClientUDP::sendTick() {
    s32 ret = mProtocal.sendData("hi", 3);
    if(0 == ret) {
        mTickTime = 0;
        mStatus = ENET_RECEIVE;
        IAppLogger::log(ELOG_INFO, "CNetClientUDP::run", "tick %u", mTickCount);
    }
}


void CNetClientUDP::sendBye() {
    mTickTime = 0;
    s32 ret = mProtocal.sendData("bye", 4);
    if(0 == ret) {
        IAppLogger::log(ELOG_INFO, "CNetClientUDP::sendBye", "bye");
    } else {
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::sendBye", "bye error");
    }
}


void CNetClientUDP::step(u64 iTime) {
    mProtocal.update(mUpdateTime);
    s32 ret = mProtocal.receiveData(mPacket.getPointer(), mPacket.getAllocatedSize());
    if(ret > 0) {
        mPacket.setUsed(ret);
        mTickTime = 0;
        mTickCount = 0;
        onPacket(mPacket);
    } else {
        if(mTickTime >= APP_NET_TICK_TIME) {
            mStatus = ENET_RECEIVE;
            ++mTickCount;
            if(mTickCount >= APP_NET_TICK_MAX_COUNT) { //relink
                mTickCount = 0;
                mTickTime = 0;
                mStatus = ENET_INVALID;
                IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::step", "over tick count[%s:%d]",
                    mAddressRemote.getIPString(), mAddressRemote.getPort());
            }
        } else {
            mTickTime += iTime;
        }
    }

    /////////////receive////////
    c8 buffer[APP_PER_CACHE_SIZE];
    ret = mConnector.receiveFrom(buffer, APP_PER_CACHE_SIZE, mAddressRemote);
    if(ret > 0) {
        s32 inret = mProtocal.import(buffer, ret);
        if(inret < 0) {
            APP_ASSERT(0 && "check protocal");
            IAppLogger::log(ELOG_ERROR, "CNetClientUDP::step", "protocal import error: [%d]", inret);
        }
    } else if(0 == ret) {
        mStatus = ENET_DISCONNECT;
        IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::step", "server quit[%s:%d]",
            mAddressRemote.getIPString(), mAddressRemote.getPort());
    } else if(ret < 0) {
        if(clearError()) {
            mStatus = ENET_TIMEOUT;
        } else {
            mStatus = ENET_ERROR;
        }
    }
}


bool CNetClientUDP::bindLocal() {
    if(mConnector.setBlock(false)) {
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::bindLocal", "set socket unblocked fail");
    }

    if(mConnector.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::bindLocal", "bind socket error: %d", mConnector.getError());
        return false;
    }
    mConnector.getLocalAddress(mAddressLocal);
    mAddressLocal.reverse();
    IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::bindLocal", "local: [%s:%d]", mAddressLocal.getIPString(), mAddressLocal.getPort());
    return true;
}


APP_INLINE void CNetClientUDP::onPacket(CNetPacket& it) {
    if(mReceiver) {
        mReceiver->onReceive(mAddressRemote, 0, it.getReadPointer(), it.getReadSize());
    }
}

}// end namespace net
}// end namespace irr

