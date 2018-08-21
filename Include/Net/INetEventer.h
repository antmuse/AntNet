#ifndef APP_INETEVENTER_H
#define APP_INETEVENTER_H


#include "CNetPacket.h"

namespace irr {
namespace net {

class CNetSocket;
class CNetAddress;
class INetSession;


enum ENetEventType {
    ENET_INVALID,
    ENET_CLOSED,             ///<closed
    ENET_POSTED,             ///<posted any
    ENET_STEPED,             ///<steped any
    ENET_SENT,               ///<send finished
    ENET_RECEIVED,           ///<received data
    ENET_CONNECTED,          ///<success connect
    ENET_DISCONNECTED,       ///<passive stop
    ENET_LINKED,             ///<client's TCP linked
    ENET_ERROR,
    ENET_CONNECT_TIMEOUT,    ///<Can't connect
    ENET_SEND_TIMEOUT,
    ENET_RECEIVE_TIMEOUT,
    ENET_UNSUPPORT,
    ENET_ET_COUNT
};

const c8* const AppNetEventTypeNames[] = {
    "NetInit",
    "NetClosed",
    "NetPosted",
    "NetSteped",
    "NetSent",
    "NetReceived",
    "NetConnected",
    "NetDisconnected",
    "NetLinked",
    "NetError",
    "NetConnectTimeout",
    "NetSendTimeout",
    "NetReceiveTimeout",
    "NetUnsupport",
    0
};


struct SNetEvent {
    struct SData {
        void* mBuffer;
        s32 mSize;
    };
    struct SSession {
        const CNetSocket* mSocket;
        const CNetAddress* mAddressLocal;
        const CNetAddress* mAddressRemote;
        INetSession* mContext;
    };
    union UEventInfo {
        SData mData;
        SSession mSession;
    };

    ENetEventType mType;
    UEventInfo mInfo;
};


class INetEventer {
public:
    INetEventer() {
    }

    virtual ~INetEventer() {
    }


    /**
    *@brief Called when net event.
    *@param iEvent The event.
    */
    virtual s32 onEvent(SNetEvent& iEvent) = 0;
};


}// end namespace net
}// end namespace irr

#endif //APP_INETEVENTER_H
