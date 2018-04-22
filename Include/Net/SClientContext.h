#ifndef APP_SCLIENTCONTEXT_H
#define APP_SCLIENTCONTEXT_H

#include "HNetOperationType.h"
#include "INetServer.h"
#include "irrArray.h"
#include "CNetPacket.h"
#include "CNetProtocal.h"
#include "CNetSocket.h"
//#include "CNetAddress.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <Windows.h>
#include <MSWSock.h>
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <netinet/in.h>
#endif


namespace irr {
namespace net {

/// Socket contex of each client
struct SClientContextUDP {
    u64 mNextTime;
    u64 mOverTime;
    CNetAddress mClientAddress;                       ///<Client address
    CNetProtocal mProtocal;
    CNetPacket  mNetPack;								 ///<The receive cache.

    SClientContextUDP();

    ~SClientContextUDP() {
    }
};

}//namespace net
}//namespace irr


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {

///Socket IO contex
struct SContextIO {
    enum EFlag {
        EFLAG_DISCONNECT = TF_DISCONNECT,
        EFLAG_REUSE = TF_REUSE_SOCKET
    };

    EOperationType mOperationType;   ///<Current operation type.
    u32 mID;
    DWORD mFlags;                      ///<send or receive flags
    DWORD mBytes;                      ///<Bytes Transferred
    OVERLAPPED   mOverlapped;        ///<Used for each operation of every socket
    WSABUF       mBuffer;			 ///<Pointer & cache size, point to the real cache.

    SContextIO() {
        init();
    }
    ~SContextIO() {
    }
    void init() {
        mFlags = 0;
        mOperationType = EOP_INVALID;
        mBytes = 0;
        ::memset(&mOverlapped, 0, sizeof(mOverlapped));
        ::memset(&mBuffer, 0, sizeof(mBuffer));
    }
};


///Socket contex of each client
struct SClientContext {
    bool mPackComplete;								///<Pack
    bool mSendPosted;
    bool mReceivePosted;
    u64 mOverTime;                                  ///<OverTime
    u32 mPacketSize;                                ///<Current packet is not full
    CNetSocket mSocket;						///<Socket of client
    CNetAddress mAddressRemote;                     ///<remote address
    CNetAddress mAddressLocal;                      ///<local address
    SQueueRingFreelock* mReadHandle;				///<Send cache read handle
    SQueueRingFreelock* mWriteHandle;				///<Send cache write handle
    SQueueRingFreelock* mInnerData;					///<Inner data list for send
    CNetPacket  mNetPack;							///<The receive cache.
    SContextIO mSender;								///<IO handles
    SContextIO mReceiver;							///<IO handles
    core::array<SContextIO> mAllContextIO;			///<All IO handles of others, eg: Accpet IO

    SClientContext();

    ~SClientContext();

    void setCache(s32 iCount, s32 perSize = 1024 - sizeof(SQueueRingFreelock));
    SQueueRingFreelock* createNetpack(s32 iSize);
    void releaseNetpack(SQueueRingFreelock* pack);
};

}// end namespace net
}// end namespace irr
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
namespace irr {
namespace net {

struct SClientContext;

///Socket IO contex
struct SContextIO {
    u32 mID;
    EOperationType mOperationType;      				///<Current operation type.
    SContextIO() : mOperationType(EOP_INVALID) {
    }

    ~SContextIO() {
    }
};


///Socket contex of each client
struct SClientContext {
    CNetSocket mSocket;												///<Socket of client
    CNetAddress mAddressRemote;                           	///<Client address
    SContextIO mSendIO;
    SContextIO mReceiveIO;
    //core::array<SContextIO*> mAllIOContext;
    CNetPacket mSendPack;
    CNetPacket mReceivePack;
    SClientContext() {
        mSendIO.mOperationType = EOP_SEND;
        mReceiveIO.mOperationType = EOP_RECEIVE;
    }
};


}// end namespace net
}// end namespace irr
#endif


#endif //APP_SCLIENTCONTEXT_H
