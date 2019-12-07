#include "CNetServerTCP.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetUtility.h"


namespace irr {
namespace net {

CNetServerTCP::CNetServerTCP() :
    mAddressLocal(APP_NET_ANY_IP, APP_NET_ANY_PORT),
    mPacket(1024),
    mStatus(ENET_INVALID),
    mThread(nullptr),
    mReceiver(nullptr),
    mReceiver2(nullptr),
    mRunning(0) {
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
            mAddressLocal.getIPString(), mAddressLocal.getPort());
        return;
    }

    if(mSocket.listen(1)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "listen error: [%s:%u]",
            mAddressLocal.getIPString(), mAddressLocal.getPort());
        return;
    }

    /*if(mSocket.setBlock(false)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "setBlock error: [%s:%u]",
            mAddressLocal.getIPString(), mAddressLocal.getPort());
        return;
    }*/

    if(mSocket.setReceiveOvertime(1000)) {
        IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "setReceiveOvertime error: [%s:%u]",
            mAddressLocal.getIPString(), mAddressLocal.getPort());
        return;
    }

    while(1 == mRunning) {
        switch(mStatus) {
        case ENET_RECEIVE:
        {
            s32 ret = mSocketSub.receive(mPacket.getWritePointer(), mPacket.getWriteSize());
            if(ret > 0) {
                if(mReceiver2) {
                    ret = mReceiver2->onReceive(mAddressRemote, 0, mPacket.getReadPointer(), mPacket.getReadSize());
                    mPacket.clear(ret);
                } else {
                    mPacket.setUsed(0);
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
        case ENET_INVALID:
        {
            mSocketSub = mSocket.accept(mAddressRemote);
            if(mSocketSub.isOpen()) {
                mAddressRemote.reverse();
                IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "client incoming: [%s:%u]",
                    mAddressRemote.getIPString(), mAddressRemote.getPort());

                if(mSocketSub.setReceiveOvertime(1000)) {
                    IAppLogger::log(ELOG_ERROR, "CNetServerTCP::run", "setReceiveOvertime error: [%s:%u]",
                        mAddressLocal.getIPString(), mAddressLocal.getPort());
                }
                mStatus = ENET_RECEIVE;
                mPacket.setUsed(0);
                if(mReceiver) {
                    mReceiver2 = mReceiver->onAccept(mAddressLocal);
                    if(mReceiver2) {
                        mReceiver2->onLink(0, mAddressLocal, mAddressLocal);
                    }
                }
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
        case ENET_DISCONNECT:
        {
            IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "client quit: [%s:%u]",
                mAddressRemote.getIPString(), mAddressRemote.getPort());
            if(mReceiver2) {
                mReceiver2->onDisconnect(0, mAddressLocal, mAddressRemote);
            }
            mSocketSub.close();
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
    }//while

    //IAppLogger::log(ELOG_CRITICAL, "CNetServerTCP::run", "server exiting, please wait...");
    mSocketSub.close();
    mSocket.close();
}


bool CNetServerTCP::clearError() {
    s32 ecode = mSocketSub.getError();
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

    IAppLogger::log(ELOG_ERROR, "CNetServerTCP::clearError", "socket error: %d", ecode);
    return false;
}


bool CNetServerTCP::start() {
    if(0 != mRunning) {
        return false;
    }
    mRunning = 1;
    if(0 == mThread) {
        mThread = new CThread();
    }
    mThread->start(*this);
    //printf("@Server started:  Listening...\n");
    return true;
}


bool CNetServerTCP::stop() {
    if(0 == mRunning) {
        return false;
    }
    mRunning = 2;
    if(mThread) {
        mThread->join();
        delete mThread;
        mThread = nullptr;
    }
    mStatus = ENET_INVALID;
    mRunning = 0;
    return true;
}


s32 CNetServerTCP::send(const c8* iData, s32 iLength) {
    if(1 != mRunning || !iData) {
        return 0;
    }
    return mSocketSub.send(iData, iLength);
}


}// end namespace net
}// end namespace irr