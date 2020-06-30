#include "CNetClientTCP.h"
#include "CNetUtility.h"
#include "HAtomicOperator.h"
#include "CLogger.h"
#include "CNetPacket.h"
#include "CThread.h"


namespace app {
namespace net {

CNetClientTCP::CNetClientTCP() :
    mThread(nullptr),
    mLastUpdateTime(0),
    mReceiver(0),
    mStatus(ENET_INVALID),
    mRunning(0) {
    CNetUtility::loadSocketLib();
}


CNetClientTCP::~CNetClientTCP() {
    stop();
    CNetUtility::unloadSocketLib();
}


bool CNetClientTCP::update(s64 iTime) {
    if(1 != mRunning) {
        APP_LOG(ELOG_DEBUG, "CNetClientTCP::update", "%s", "not running");
        return true;
    }

    const s64 iStepTime = iTime > mLastUpdateTime ? iTime - mLastUpdateTime : 0; //unit: ms
    mLastUpdateTime = iTime;

    switch(mStatus) {
    case ENET_RECEIVE:
    {
        s32 ret = mConnector.receive(mPacket.getWritePointer(), mPacket.getWriteSize());
        if(ret > 0) {
            if(mReceiver) {
                mPacket.setUsed(mPacket.getSize() + ret);
                ret = mReceiver->onReceive(mAddressRemote, 0, mPacket.getReadPointer(), mPacket.getReadSize());
                mPacket.clear(ret);
            }
        } else if(0 == ret) {
            mStatus = ENET_DISCONNECT;
        } else if(clearError()) {
            mStatus = ENET_TIMEOUT;
        } else {
            mStatus = ENET_ERROR;
        }
        break;
    }
    case ENET_TIMEOUT:
    {
        if(mReceiver) {
            mReceiver->onTimeout(0, mAddressLocal, mAddressRemote);
        }
        mStatus = ENET_RECEIVE;
        break;
    }
    case ENET_INVALID:
    {
        resetSocket();
        mStatus = connect() ? ENET_RECEIVE : ENET_ERROR;
        mPacket.setUsed(0);
        break;
    }
    case ENET_DISCONNECT:
    {
        if(mReceiver) {
            s32 ret = mReceiver->onDisconnect(0, mAddressLocal, mAddressRemote);
            if(0 == ret) {
                AppAtomicFetchCompareSet(2, 1, (s32*) &mRunning);
            }
        }
        mStatus = ENET_INVALID;
        break;
    }
    case ENET_ERROR:
    default:
    {
        mStatus = ENET_DISCONNECT;
        CThread::sleep(1000);
        break;
    }
    }//switch
    return true;
}


bool CNetClientTCP::connect() {
    if(0 == mConnector.connect(mAddressRemote)) {
        mConnector.getLocalAddress(mAddressLocal);
        if(mReceiver) {
            mReceiver->onConnect(0, mAddressLocal, mAddressRemote);
        }
        return true;
    }

    CLogger::log(ELOG_CRITICAL, "CNetClientTCP::connect", "can't connect with server: [%s:%d] [ecode=%d]",
        mAddressRemote.getIPString(), mAddressRemote.getPort(),
        mConnector.getError());
    //mThread->sleep(5000);

    return false;
}


void CNetClientTCP::resetSocket() {
    mPacket.clear(0xFFFFFFFF);
    mConnector.close();
    if(mConnector.openTCP()) {
        mConnector.setReceiveOvertime(1000);
    } else {
        CLogger::log(ELOG_ERROR, "CNetClientTCP::resetSocket", "create socket fail");
    }
}


bool CNetClientTCP::clearError() {
    s32 ecode = mConnector.getError();

    switch(ecode) {
#if defined(APP_PLATFORM_WINDOWS)
    case WSAETIMEDOUT:
    case WSAEWOULDBLOCK: //none data
        return true;
    case WSAECONNRESET: //reset
        break;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        //case EAGAIN:
    case EWOULDBLOCK: //none data
        return true;
    case ECONNRESET: //reset
        break;
#endif
    default:
        return false;
    }//switch

    CLogger::log(ELOG_ERROR, "CNetClientTCP::clearError", "socket error: %d", ecode);
    return false;
}


void CNetClientTCP::run() {
    while(1 == mRunning) {
        update(0);
        //CThread::sleep(300);
    }
    mRunning = 3;
}


bool CNetClientTCP::start(bool useThread) {
    if(0 != mRunning) {
        return false;
    }
    mRunning = 1;
    if(useThread) {
        mThread = new CThread();
        mThread->start(*this);
    }
    CLogger::log(ELOG_INFO, "CNetClientTCP::start", "success");
    return true;
}


bool CNetClientTCP::stop() {
    if(0 == mRunning) {
        return false;
    }
    AppAtomicFetchCompareSet(2, 1, (s32*)&mRunning);
    mConnector.close();
    if(mThread) {
        mThread->join();
        delete mThread;
        mThread = nullptr;
    }
    mStatus = ENET_INVALID;
    mRunning = 0;
    CLogger::log(ELOG_INFO, "CNetClientTCP::stop", "success");
    return true;
}


s32 CNetClientTCP::send(const void* iData, s32 iLength) {
    if(1 != mRunning || !iData) {
        CLogger::log(ELOG_ERROR, "CNetClientTCP::send",
            "status: %d", mStatus);
        return 0;
    }

    return mConnector.send(iData, iLength);
}


}// end namespace net
}// end namespace app

