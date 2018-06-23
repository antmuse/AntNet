#include "CNetClientUDP.h"
#include "CNetUtility.h"
#include "CThread.h"
#include "IAppLogger.h"


#define APP_PER_CACHE_SIZE	(1024)


namespace irr {
namespace net {

CNetClientUDP::CNetClientUDP() :
    mAddressLocal(APP_NET_ANY_IP, APP_NET_ANY_PORT),
    mStatus(ENM_RESET),
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


s32 CNetClientUDP::sendData(const c8* iData, s32 iLength) {
    switch(mStatus) {
    case ENM_HELLO:
        return 0;
    default:
        if(ENM_WAIT != mStatus) {
            return -1;
        }
    }
    return mProtocal.sendData(iData, iLength);
}


bool CNetClientUDP::clearError() {
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
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::clearError", "server reseted: %d", ecode);
        return false;
    default:
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::clearError", "socket error: %d", ecode);
        return false;
    }//switch

    return false;
}


bool CNetClientUDP::update(u64 iTime) {
    u64 steptime = iTime - mUpdateTime;
    mUpdateTime = iTime;

    switch(mStatus) {
    case ENM_WAIT:
        step(steptime);
        break;

    case ENM_HELLO:
        sendTick();
        break;

    case ENM_RESET:
        resetSocket();
        break;

    case ENM_BYE:
        sendBye();
        mProtocal.flush();
        mRunning = false;
        mConnector.close();
        return false;

    default:
        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::run", "Default? How can you come here?");
        break;

    }//switch

    return true;
}


bool CNetClientUDP::start() {
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

    mStatus = ENM_BYE; //mRunning = false
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
        mStatus = ENM_HELLO;
        mProtocal.flush();
        IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::resetSocket", "reused previous socket");
    }
}


void CNetClientUDP::sendTick() {
    s32 ret = mProtocal.sendData("hi", 3);
    if(0 == ret) {
        mTickTime = 0;
        mStatus = ENM_WAIT;
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
            mStatus = ENM_HELLO;
            ++mTickCount;
            if(mTickCount >= APP_NET_TICK_MAX_COUNT) { //relink
                mTickCount = 0;
                mTickTime = 0;
                mStatus = ENM_RESET;
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
    } else if(ret < 0) {
        if(!clearError()) {
            mStatus = ENM_RESET;
            IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::step", "server maybe quit[%s:%d]",
                mAddressRemote.getIPString(), mAddressRemote.getPort());
        }
    } else if(0 == ret) {
        mStatus = ENM_RESET;
        IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::step", "server quit[%s:%d]",
            mAddressRemote.getIPString(), mAddressRemote.getPort());
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
    if(mReceiver) mReceiver->onReceive(it);
}

}// end namespace net
}// end namespace irr

