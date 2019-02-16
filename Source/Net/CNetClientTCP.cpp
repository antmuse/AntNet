#include "CNetClientTCP.h"
#include "CNetUtility.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "CThread.h"
#include "IUtility.h"


#define APP_MAX_CACHE_NODE	(10)
#define APP_PER_CACHE_SIZE	(512)


namespace irr {
namespace net {

CNetClientTCP::CNetClientTCP() :
    //mPacket(128),
    mStream(APP_MAX_CACHE_NODE, APP_PER_CACHE_SIZE),
    mPacketSize(0),
    mTickTime(0),
    mLastUpdateTime(0),
    mTickCount(0),
    mReceiver(0),
    mStatus(ENM_RESET),
    mRunning(false) {
    CNetUtility::loadSocketLib();
}


CNetClientTCP::~CNetClientTCP() {
    stop();
    CNetUtility::unloadSocketLib();
}


bool CNetClientTCP::update(u64 iTime) {
    if(!mRunning) {
        APP_LOG(ELOG_DEBUG, "CNetClientTCP::update", "%s", "not running");
        return true;
    }

    const u64 iStepTime = iTime > mLastUpdateTime ? iTime - mLastUpdateTime : 0; //unit: ms
    mLastUpdateTime = iTime;

    switch(mStatus) {
    case ENM_WAIT:
    {
        flushDatalist();
        s32 ret = (0 == mPacketSize) ?
            APP_NET_PACKET_HEAD_SIZE - mPacket.getSize()
            : mPacketSize - mPacket.getSize();
        ret = mConnector.receive(mPacket.getWritePointer(), ret);

        if(ret > 0) {
            mTickTime = 0;
            mTickCount = 0;
            const u32 psize = mPacket.getSize() + ret;
            mPacket.setUsed(psize);
            if(0 == mPacketSize) {
                if(psize >= APP_NET_PACKET_HEAD_SIZE) {
                    utility::AppDecodeU32(mPacket.getPointer(), &mPacketSize);
                    mPacket.setUsed(0);
                    mPacketSize -= APP_NET_PACKET_HEAD_SIZE;
                    mPacket.reallocate(mPacketSize);
                }
                return true;
            }

            if(psize < mPacketSize) {
                return true;
            }

            if(psize == sizeof(u8)) {
                u8 bit = mPacket.readU8();
                APP_LOG(ELOG_CRITICAL, "CNetClientTCP::update",
                    "remote msg[%s]", AppNetMessageNames[bit]);
            } else {
                if(mReceiver) {
                    mReceiver->onReceive(0, mPacket.getReadPointer(), mPacket.getReadSize());
                }
            }
            mPacketSize = 0;
            mPacket.setUsed(0);
        } else if(0 == ret) {
            mStatus = ENM_RESET;
            IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::update", "remote quit[%s:%d]",
                mAddressRemote.getIPString(), mAddressRemote.getPort());
        } else {
            if(!clearError()) {
                mStatus = ENM_RESET;
                break;
            }
            if(mTickTime >= APP_NET_TICK_TIME) {
                mStatus = ENM_HELLO;
                mTickTime = 0;
                ++mTickCount;

                if(mTickCount >= APP_NET_TICK_MAX_COUNT) { //relink
                    mTickCount = 0;
                    mTickTime = 0;
                    mStatus = ENM_RESET;
                    //IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::update", "over tick count[%s:%d]", mIP.c_str(), mPort);
                }
            } else {
                mTickTime += iStepTime;
                //mThread->sleep(iStepTime);
                //printf(".");
            }
        }
        break;
    }
    case ENM_HELLO:
    {
        sendTick();
        break;
    }
    case ENM_LINK:
    {
        if(mTickTime < APP_NET_CONNECT_TIME_INTERVAL) {
            mTickTime += iStepTime;
        } else {
            connect();
            mTickTime = 0;
        }
        break;
    }
    case ENM_RESET:
    {
        resetSocket();
        mStream.clear();
        mPacketSize = 0;
        mPacket.setUsed(0);
        mTickTime = APP_NET_CONNECT_TIME_INTERVAL;
        break;
    }
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "Default? How can you come here?");
        return false;   //tell manager to remove this connection.
    }
    }//switch

    return true;
}


void CNetClientTCP::sendTick() {
    if(mStream.isEmpty()) {
        const u32 psize = sizeof(u32) + 1;
        c8 msg[psize];
        utility::AppEncodeU32(psize, msg);
        msg[psize - 1] = ENM_HELLO;
        mStream.write(msg, psize);
    } else {
        IAppLogger::log(ELOG_ERROR, "CNetClientTCP::sendTick", 
            "send stream is not empty:[ecode=%u]", mConnector.getError());
    }
    mStatus = ENM_WAIT;
}


void CNetClientTCP::connect() {
    if(0 == mConnector.connect(mAddressRemote)) {
        if(mConnector.setBlock(false)) {
            IAppLogger::log(ELOG_ERROR, "CNetClientTCP::connect", "set socket unblocked fail: [%d]", mConnector.getError());
        }

        mConnector.getLocalAddress(mAddressLocal);
        mStatus = ENM_HELLO;
    } else {
        IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::connect", "can't connect with server: [%s:%d] [ecode=%d]",
            mAddressRemote.getIPString(), mAddressRemote.getPort(),
            mConnector.getError());
        //mThread->sleep(5000);
    }
}


void CNetClientTCP::resetSocket() {
    mConnector.close();
    if(mConnector.openTCP()) {
        mTickTime = 0;
        mTickCount = 0;
        mStatus = ENM_LINK;
        //IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::resetSocket", "create socket success");
    } else {
        IAppLogger::log(ELOG_ERROR, "CNetClientTCP::resetSocket", "create socket fail");
    }
}


bool CNetClientTCP::clearError() {
    s32 ecode = mConnector.getError();

    switch(ecode) {
#if defined(APP_PLATFORM_WINDOWS)
        //case WSAGAIN:
    case WSAEWOULDBLOCK: //none data
        return true;
    case WSAECONNRESET: //reset
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        //case EAGAIN:
    case EWOULDBLOCK: //none data
        return true;
    case ECONNRESET: //reset
#endif
        IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "server reseted: %d", ecode);
        return false;
    default:
        IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "socket error: %d", ecode);
        return false;
    }//switch

    return false;
}


bool CNetClientTCP::flushDatalist() {
    bool successed = true;
    s32 sentsize;
    s32 packsize;
    s32 ret;						//socket function return value.
    c8* spos;			        //send position in current pack.

    for(; mStream.openRead(&spos, &packsize); ) {
        sentsize = 0;

        while(sentsize < packsize) {
            ret = mConnector.send(spos, packsize - sentsize);
            if(ret > 0) {
                sentsize += ret;
                spos += ret;
            } else if(0 == ret) {
                IAppLogger::log(ELOG_INFO, "CNetClientTCP::flushDatalist", "send cache full");
                break;
            } else {
                if(!clearError()) {
                    mStatus = ENM_RESET;
                }
                break;
            }
        }//while

        if(sentsize > 0) {
            mStream.closeRead(sentsize);
            mTickTime = 0;
            mTickCount = 0;
        }

        if(sentsize < packsize) {//maybe socket cache full
            successed = false;
            break;
        }
    }//for

     //APP_LOG(ELOG_INFO, "CNetClientTCP::flushDatalist", "flush [%s]", successed ? "success" : "fail");
    return successed;
}


void CNetClientTCP::setNetPackage(ENetMessage it, net::CNetPacket& out) {
    out.setUsed(sizeof(u8));
    out[0] = it;
}


bool CNetClientTCP::start() {
    if(mRunning) {
        return false;
    }
    mRunning = true;
    IAppLogger::log(ELOG_INFO, "CNetClientTCP::start", "success");
    return true;
}


bool CNetClientTCP::stop() {
    if(!mRunning) {
        return false;
    }
    mRunning = false;
    mStream.clear();
    mConnector.close();
    IAppLogger::log(ELOG_INFO, "CNetClientTCP::stop", "success");
    return true;
}


s32 CNetClientTCP::sendData(const c8* iData, s32 iLength) {
    if(ENM_WAIT != mStatus || !mRunning || !iData) {
        IAppLogger::log(ELOG_ERROR, "CNetClientTCP::sendData", "invalid: %s", AppNetMessageNames[mStatus]);
        return -1;
    }

    u32 psize = sizeof(u32) + iLength;

    while(!mStream.isEnough(psize)) {
        CThread::yield();
        if(ENM_WAIT != mStatus) {
            IAppLogger::log(ELOG_ERROR, "CNetClientTCP::sendData", "stream full and net delink");
            return 0;
        }
    }

    utility::AppEncodeU32(psize, (c8*) &psize);
    mStream.write((c8*) &psize, sizeof(psize), false, false);
    return mStream.write(iData, iLength, false, true);
}


}// end namespace net
}// end namespace irr

