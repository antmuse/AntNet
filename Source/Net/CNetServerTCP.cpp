#include "CNetServerTCP.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetUtility.h"


namespace irr {
namespace net {

#define APP_MAX_CACHE_NODE	(10)
#define APP_PER_CACHE_SIZE	(512)

CNetServerTCP::CNetServerTCP() :
    mStream(APP_MAX_CACHE_NODE, APP_PER_CACHE_SIZE),
    mAddressLocal(APP_NET_ANY_IP, APP_NET_ANY_PORT),
    mPacket(1024),
    mStatus(ENM_RESET),
    mReceiver(0),
    mThread(0),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetServerTCP::~CNetServerTCP() {
    delete mThread;
    CNetUtility::unloadSocketLib();
}


void CNetServerTCP::run() {
    if(!mSocket.openTCP()) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "create socket error!!!");
        return;
    }

    if(mSocket.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "bind error: [%s:%u]",
            mAddressLocal.getIPString().c_str(), mAddressLocal.getPort());
        return;
    }

    if(mSocket.listen(1)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "listen error: [%s:%u]",
            mAddressLocal.getIPString().c_str(), mAddressLocal.getPort());
        return;
    }

    if(mSocket.setBlock(false)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "setBlock error: [%s:%u]",
            mAddressLocal.getIPString().c_str(), mAddressLocal.getPort());
        return;
    }

    mStatus = ENM_RESET;

    s32 oldsize;
    s32 ret;
    while(mRunning) {
        switch(mStatus) {
        case ENM_WAIT:
            oldsize = mPacket.getSize();
            ret = mSocketSub.receive(mPacket.getWritePointer(), mPacket.getWriteSize());
            if(ret > 0) {
                mPacket.setUsed(oldsize + ret);
                if(mReceiver) {
                    mReceiver->onReceive(mPacket);
                }
            } else if(0 == ret) {
                IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "client quit: [%s:%u]",
                    mAddressRemote.getIPString().c_str(), mAddressRemote.getPort());
                mSocketSub.close();
                mStatus = ENM_RESET;
            } else {
                IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "client quit abnormal: [%s:%u]",
                    mAddressRemote.getIPString().c_str(), mAddressRemote.getPort());
                mSocketSub.close();
                mStatus = ENM_RESET;
            }
            break;

        case ENM_RESET:
            mSocketSub = mSocket.accept(mAddressRemote);
            if(mSocketSub.isOpen()) {
                mAddressRemote.reverse();
                IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "client incoming: [%s:%u]", 
                    mAddressRemote.getIPString().c_str(), mAddressRemote.getPort());
                mStatus = ENM_WAIT;
                mPacket.setUsed(0);
                if(mReceiver) {
                    mReceiver->onEvent(ENET_CONNECTED);
                }
            }
            break;
        }//switch

        mThread->sleep(50);
    }//while

    //IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "server exiting, please wait...");
    mSocket.close();
}


bool CNetServerTCP::start() {
    if(mRunning) {
        return false;
    }
    mRunning = true;
    if(0 == mThread) {
        mThread = new CThread();
    }
    mThread->start(*this);
    //printf("@Server started:  Listening...\n");
    return true;
}


bool CNetServerTCP::stop() {
    if(!mRunning) {
        return false;
    }
    mRunning = false;
    mThread->join();
    return true;
}


s32 CNetServerTCP::sendData(const c8* iData, s32 iLength) {
    if(!mRunning || !iData || ENM_WAIT != mStatus) {
        return -1;
    }

    s32 ret = 0;
    s32 step;

    for(; ret < iLength;) {
        step = mSocketSub.send(iData + ret, iLength - ret);
        if(step > 0) {
            ret += step;
        } else {
            break;// CThread::yield();
        }
    }

    return ret;
}


}// end namespace net
}// end namespace irr