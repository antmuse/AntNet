#ifndef APP_SCLIENTCONTEXT_H
#define APP_SCLIENTCONTEXT_H

#include "HNetOperationType.h"
#include "INetServer.h"
#include "AppArray.h"
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


namespace app {
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
}//namespace app


#if defined(APP_PLATFORM_WINDOWS)
namespace app {
namespace net {

///Socket IO contex
struct SContextIO {
    enum EFlag {
        EFLAG_DISCONNECT = TF_DISCONNECT,
        EFLAG_REUSE = TF_REUSE_SOCKET
    };

    OVERLAPPED mOverlapped;        ///<Used for each operation of every socket
    EOperationType mOperationType;   ///<Current operation type.
    u32 mID;
    DWORD mFlags;            ///<send or receive flags
    DWORD mBytes;            ///<Bytes Transferred
    WSABUF mBuffer;			 ///<Pointer & cache size, point to the real cache.

    void clear() {
        ::memset(this, 0, sizeof(*this));
    }
};

}// end namespace net
}// end namespace app


#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )

namespace app {
namespace net {

///Socket IO contex
struct SContextIO {
    u32 mID;
    EOperationType mOperationType;  ///<Current operation type.
    SContextIO() : mOperationType(EOP_INVALID) {
    }

    ~SContextIO() {
    }
};

}// end namespace net
}// end namespace app
#endif

#endif //APP_SCLIENTCONTEXT_H
