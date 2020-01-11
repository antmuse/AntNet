#ifndef APP_INETEVENTER_H
#define APP_INETEVENTER_H


#include "HNetConfig.h"
#include "CNetPacket.h"

namespace irr {
namespace net {

class CNetSocket;
class CNetAddress;
class INetEventer;

enum ENetEventType {
    ENET_INVALID,
    ENET_SEND,               ///<request launch send
    ENET_RECEIVE,            ///<request launch receive
    ENET_CONNECT,            ///<request launch connect
    ENET_DISCONNECT,         ///<request launch disconnect
    ENET_LINKED,             ///<for server, a client coming
    ENET_TIMEOUT,
    ENET_ERROR,
    ENET_ET_COUNT
};

const c8* const AppNetEventTypeNames[] = {
    "NetInvalid",
    "NetSend",
    "NetReceive",
    "NetConnect",
    "NetDisconnect",
    "NetLinked",
    "NetTimeout",
    "NetError",
    0
};


struct SNetEvent {
    struct SDataSend {
        s32 mAllocatedSize;
        s32 mSize;
        void* mBuffer;
    };
    struct SDataReceive {
        s32 mAllocatedSize;
        s32 mSize;
        void* mBuffer;
    };
    struct SSessionInfo {
        const CNetAddress* mAddressLocal;
        const CNetAddress* mAddressRemote;
    };
    union UEventInfo {
        SDataSend mDataSend;
        SDataReceive mDataReceive;
        SSessionInfo mSession;
    };
    ENetEventType mType;
    u32 mSessionID;
    UEventInfo mInfo;
};


class INetEventer {
public:
    INetEventer() {
    }

    virtual ~INetEventer() {
    }

    virtual INetEventer* onAccept(const CNetAddress& local) = 0;

    /**
    *@brief Called when connected.
    *@param sessionID The ID of session.
    *@param sock The socket of session.
    */
    virtual s32 onConnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote) = 0;

    virtual s32 onLink(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote) = 0;

    virtual s32 onDisconnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote) = 0;

    virtual s32 onTimeout(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote) = 0;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result) = 0;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) = 0;
};


}// end namespace net
}// end namespace irr

#endif //APP_INETEVENTER_H
