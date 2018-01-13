#ifndef APP_INETEVENTER_H
#define APP_INETEVENTER_H


#include "CNetPacket.h"

namespace irr {
namespace net {

enum ENetEventType {
    ENET_INVALID,
    ENET_CLOSED,             ///<closed
    ENET_POSTED,             ///<posted any
    ENET_STEPED,             ///<steped any
    ENET_SENT,               ///<send finished
    ENET_RECEIVED,           ///<received data
    ENET_CONNECTED,          ///<success connect
    ENET_DISCONNECTED,       ///<passive stop
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
        u32 mSize;
    };

    ENetEventType mType;
    union {
        SData mData;
    };
};


class INetEventer {
public:
    INetEventer() {
    }

    virtual ~INetEventer() {
    }

    /**
    *@brief Callback function when received net packet.
    *@param The received package.
    */
    virtual void onReceive(CNetPacket& it) = 0;

    /**
    *@brief Called when net status changed.
    *@param iEvent The event type.
    */
    virtual void onEvent(ENetEventType iEvent) = 0;
};


}// end namespace net
}// end namespace irr

#endif //APP_INETEVENTER_H
