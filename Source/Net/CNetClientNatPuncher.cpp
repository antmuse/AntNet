#include "CNetClientNatPuncher.h"
#include "CNetUtility.h"
#include "CThread.h"
#include "IAppLogger.h"


#define APP_PER_CACHE_SIZE	(512)


namespace irr {
namespace net {

CNetClientNatPuncher::CNetClientNatPuncher() :
    mStatus(ENM_RESET),
    mPacket(APP_PER_CACHE_SIZE),
    mUpdateTime(0),
    mReceiver(0),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetClientNatPuncher::~CNetClientNatPuncher() {
    stop();
    CNetUtility::unloadSocketLib();
}


s32 CNetClientNatPuncher::sendBuffer(void* iUserPointer, const c8* iData, s32 iLength) {
    s32 ret = 0;
    s32 sent;
    for(; ret < iLength;) {
        sent = mConnector.sendto(iData + ret, iLength - ret, mAddressPeer);
        if(sent > 0) {
            ret += sent;
        } else {
            if(!clearError()) {
                break;
            }
            CThread::yield();
        }
    }
    APP_LOG(ELOG_INFO, "CNetClientNatPuncher::sendBuffer", "Sent leftover: [%d - %d]", iLength, ret);
    return ret;
}


s32 CNetClientNatPuncher::sendData(const c8* iData, s32 iLength) {
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


bool CNetClientNatPuncher::clearError() {
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
        IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::clearError", "server reseted: %d", ecode);
        return false;
    default:
        IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::clearError", "socket error: %d", ecode);
        return false;
    }//switch

    return false;
}


bool CNetClientNatPuncher::update(u64 iTime) {
    u64 steptime = iTime - mUpdateTime;
    mUpdateTime = iTime;
    s32 ret;

    switch(mStatus) {
    case ENM_WAIT:
        mProtocal.sendData("hello peer", 11);
        step(steptime);
        break;

    case ENM_LINK:
        break;

    case ENM_NAT_PUNCH:
    {
        if(mPunchCount >= 0) {
            c8 lk = ENM_LINK;
            mConnector.sendto(&lk, sizeof(lk), mAddressPeer);
            --mPunchCount;
        } else {
            mPunchCount = 5;
        }
        ret = mConnector.receiveFrom(mPacket.getPointer(), mPacket.getAllocatedSize(), mAddressRemote);
        if(ret > 0) {
            mPacket.setUsed(ret);
            switch(mPacket.readU8()) {
            case ENM_LINK:
            {
                mAddressRemote.reverse();
                IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::run",
                    "success, peer address: [%s:%u]",
                    mAddressRemote.getIPString(),
                    mAddressRemote.getPort());

                mStatus = ENM_WAIT;
                break;
            }
            }//switch
        } else if(0 == ret) {
            //mStatus = ENM_HELLO;
        } else if(!clearError()) {
            IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::run", "punch error: %u", mPunchCount);
        }
        break;
    }

    case ENM_HELLO:
    {
        c8 hi = ENM_HELLO;
        mConnector.sendto(&hi, sizeof(hi), mAddressServer);
        ret = mConnector.receiveFrom(mPacket.getPointer(), mPacket.getAllocatedSize(), mAddressRemote);
        if(ret > 0) {
            mPacket.setUsed(ret);
            switch(mPacket.readU8()) {
            case ENM_NAT_PUNCH:
            {
                mAddressPeer.setIP(mPacket.readU32());
                mAddressPeer.setPort(mPacket.readU16());
                mStatus = ENM_NAT_PUNCH;
                mPunchCount = 5;
                IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::run",
                    "peer address: [%s:%u]",
                    mAddressPeer.getIPString(),
                    mAddressPeer.getPort());
                break;
            }
            case ENM_HELLO:
            {
                CNetAddress::IP ipid = mPacket.readU32();
                core::stringc pip;
                CNetAddress::convertIPToString(ipid, pip);
                IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::run",
                    "my address: [%s:%u]",
                    pip.c_str(), mPacket.readU16());
                mPacket.setUsed(0);
                mPacket.add(u8(ENM_NAT_PUNCH));
                mPacket.add(u32(0));
                mPacket.add(u16(0));
                mConnector.sendto(mPacket.getPointer(), mPacket.getSize(), mAddressServer);
                break;
            }
            }//switch
        } else if(0 == ret) {
            mStatus = ENM_HELLO;
        } else if(!clearError()) {
            IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::run", "hi error: %u", mPunchCount);
        }
        break;
    }


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
        IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::run", "Default? How can you come here?");
        break;

    }//switch

    return true;
}


bool CNetClientNatPuncher::start() {
    if(mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::start", "client is running currently.");
        return false;
    }
    mRunning = true;
    mTickCount = 0;
    mTickTime = 0;
    mProtocal.setSender(this);
    //mStatus = ENM_RESET;
    return true;
}


bool CNetClientNatPuncher::stop() {
    if(!mRunning) {
        IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::stop", "client stoped.");
        return false;
    }

    mStatus = ENM_BYE; //mRunning = false
    return true;
}


void CNetClientNatPuncher::setID(u32 id) {
    mProtocal.setID(id);
}


void CNetClientNatPuncher::resetSocket() {
    mTickCount = 0;
    mTickTime = 0;

    mConnector.close();
    if(mConnector.openUDP()) {
        bindLocal();
        mStatus = ENM_HELLO;
        mProtocal.flush();
        IAppLogger::log(ELOG_CRITICAL, "CNetClientNatPuncher::resetSocket", "reused previous socket");
    }
}


void CNetClientNatPuncher::sendTick() {
    c8 hi = ENM_HELLO;
    mConnector.sendto(&hi, sizeof(hi), mAddressServer);
    mConnector.sendto(&hi, sizeof(hi), mAddressServer);
    mConnector.sendto(&hi, sizeof(hi), mAddressServer);
    mTickTime = 0;
    IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::sendTick", "tick %u", mTickCount);
}


void CNetClientNatPuncher::sendBye() {
    mTickTime = 0;
    s32 ret = mProtocal.sendData("bye", 4);
    if(0 == ret) {
        IAppLogger::log(ELOG_INFO, "CNetClientNatPuncher::sendBye", "bye");
    } else {
        IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::sendBye", "bye error");
    }
}


void CNetClientNatPuncher::step(u64 iTime) {
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
                IAppLogger::log(ELOG_CRITICAL, "CNetClientNatPuncher::step", "over tick count[%s:%d]",
                    mAddressServer.getIPString(), mAddressServer.getPort());
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
            IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::step", "protocal import error: [%d]", inret);
        }
    } else if(ret < 0) {
        if(!clearError()) {
            mStatus = ENM_RESET;
            IAppLogger::log(ELOG_CRITICAL, "CNetClientNatPuncher::step", "server maybe quit[%s:%d]",
                mAddressServer.getIPString(), mAddressServer.getPort());
        }
    } else if(0 == ret) {
        mStatus = ENM_RESET;
        IAppLogger::log(ELOG_CRITICAL, "CNetClientNatPuncher::step", "server quit[%s:%d]",
            mAddressServer.getIPString(), mAddressServer.getPort());
    }
}


bool CNetClientNatPuncher::bindLocal() {
    if(mConnector.setBlock(false)) {
        IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::bindLocal", "set socket unblocked fail");
    }

    if(mConnector.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetClientNatPuncher::bindLocal", "bind socket error: %d", mConnector.getError());
        return false;
    }
    mConnector.getLocalAddress(mAddressLocal);
    mAddressLocal.reverse();
    IAppLogger::log(ELOG_CRITICAL, "CNetClientNatPuncher::bindLocal", "local: [%s:%d]", mAddressLocal.getIPString(), mAddressLocal.getPort());
    return true;
}


APP_INLINE void CNetClientNatPuncher::onPacket(CNetPacket& it) {
    if(mReceiver) mReceiver->onReceive(it);
}


}// end namespace net
}// end namespace irr

