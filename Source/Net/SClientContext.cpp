#include "SClientContext.h"
#include "CNetUtility.h"


namespace irr {
namespace net {

////////////////////////////////////////////////////////////////////////////////////
SClientContextUDP::SClientContextUDP() : mNextTime(0) {
}

}//namespace net
}//namespace irr


#if defined(APP_PLATFORM_WINDOWS)

namespace irr {
namespace net {


////////////////////////////////////////////////////////////////////////////////////
SClientContext::SClientContext() : mPackComplete(true),
mSendPosted(false),
mReceivePosted(false),
mInnerData(0),
mReadHandle(0),
mWriteHandle(0),
mPacketSize(0) {
    mSender.mOperationType = EOP_SEND;
    mReceiver.mOperationType = EOP_RECEIVE;
}


void SClientContext::setCache(s32 iCount, s32 perSize/* = 1024-sizeof(SQueueRingFreelock)*/) {
    mReadHandle = createNetpack(perSize);

    if(!mReadHandle) return;

    SQueueRingFreelock* it;
    for(; iCount >= 0; --iCount) {
        it = createNetpack(perSize);
        if(!it) break;
        mReadHandle->pushBack(it);
    }
    mWriteHandle = mReadHandle;
}


SClientContext::~SClientContext() {
    mSocket.close();
    if(mReadHandle) {
        SQueueRingFreelock* next;
        for(SQueueRingFreelock* it = mReadHandle->getNext(); it != mReadHandle; it = next) {
            next = it->getNext();
            it->delink();
            releaseNetpack(it);
        }
        releaseNetpack(mReadHandle);
        mReadHandle = 0;
    }

    const u32 addsize = sizeof(SOCKADDR_IN) + 16;
    const u32 each = sizeof(netsocket) + 2 * addsize;
    netsocket sock = 0;
    u32 count = mAllContextIO.size();
    for(u32 i = 0; i < count; ++i) {
        if(EOP_ACCPET == mAllContextIO[i].mOperationType) {
            sock = *(netsocket*) (mNetPack.getPointer() + mAllContextIO[i].mID*each);
            if(APP_INVALID_SOCKET != sock) 	::closesocket(sock);
        }
    }
}


SQueueRingFreelock* SClientContext::createNetpack(s32 iSize) {
    //SQueueRingFreelock* ret = (SQueueRingFreelock*) mMemHub.allocate(iSize+sizeof(SQueueRingFreelock));
    SQueueRingFreelock* ret = (SQueueRingFreelock*) malloc(iSize + sizeof(SQueueRingFreelock));
    if(ret)	ret->init(iSize);
    return ret;
}


void SClientContext::releaseNetpack(SQueueRingFreelock* pack) {
    //mMemHub.release((u8*)pack);
    free(pack);
}

}// end namespace net
}// end namespace irr

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace irr {
namespace net {


}//namespace net
}//namespace irr
#endif
