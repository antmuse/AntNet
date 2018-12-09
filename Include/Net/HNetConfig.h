#ifndef APP_HNETCONFIG_H
#define APP_HNETCONFIG_H

#include "HConfig.h"
#include "irrTypes.h"


namespace irr {
namespace net {

//#define APP_NET_USE_IPV6


///Socket IO cache size is 8K (8192=1024*8)
#define APP_NET_MAX_BUFFER_LEN     8192
#define APP_NET_DEFAULT_PORT       9011
#define APP_NET_DEFAULT_IP         ("127.0.0.1")
#define APP_NET_ANY_IP             ("0.0.0.0")
#define APP_NET_ANY_PORT           (0)
#define APP_NET_LOCAL_HOST         ("localhost")

///Server default threads count
#define APP_NET_DEFAULT_SERVER_THREAD   2

///The tick time, we need keep the net link alive.
#define APP_NET_TICK_TIME  (30*1000)

///We will relink the socket if tick count >= APP_NET_TICK_MAX_COUNT
#define APP_NET_TICK_MAX_COUNT  (4)

///Http timeout
#define APP_NET_HTTP_TIMEOUT  (10*1000)

///connect time inteval
#define APP_NET_CONNECT_TIME_INTERVAL  (5*1000)


///Net packet head size
#define APP_NET_PACKET_HEAD_SIZE (sizeof(u32))



///Define net message types
enum ENetMessage {
    ENM_INVALID = 0,
    ENM_WAIT,
    ENM_HELLO,
    ENM_BYE,
    ENM_DATA,
    ENM_LINK,
    ENM_RESET,
    ENM_CLEAR,
    ENM_NAT_PUNCH,
    ENM_COUNT
};

///Define net message typenames
const c8* const AppNetMessageNames[] = {
    "NetInit",
    "NetWait",
    "NetHello",
    "NetBye",
    "NetData",
    "NetLink",
    "NetReset",
    "NetClear",
    "NetNatPunch",
    0
};


///Define net peer types
enum ENetNodeType {
    ENET_UNKNOWN = 0,
    ENET_TCP_CLIENT,
    ENET_UDP_CLIENT,
    ENET_TCP_SERVER_LOW,
    ENET_TCP_SERVER,
    ENET_UDP_SERVER,
    ENET_HTTP_CLIENT,
    ENET_NODE_TYPE_COUNT
};


///Define net peer typenames
const c8* const AppNetNodeNames[] = {
    "Unknown",
    "ClientTCP",
    "ClientUDP",
    "LowServerTCP",
    "ServerTCP",
    "ServerUDP",
    "ClientHTTP",
    0
};




class CNetConfig {
public:
    //socket
    bool mReuse;     ///<reuse IP and port
    bool mOnDelay;

    //range[8,64]
    u8 mMaxPostAccept;
    //range[32,512]
    u16 mMaxFatchEvents;

    u32 mLinger;
    u32 mKeepAliveIdle;
    u32 mKeepAliveInterval;
    u32 mKeepAliveMaxTick;
    u32 mSendTimeout;
    u32 mReceiveTimeout;
    u32 mSendCacheSize;
    u32 mRecieveCacheSize;

    //server
    u32 mMaxContext; ///<max session on each epoll thread
    u32 mMaxOnFly;   ///<max fly packets on each TCP
    u32 mMaxPackets; ///<max packets hold by a server
    u32 mMaxMessageSize;
    //range[1000,60000]
    u32 mPollTimeout; ///<in ms

    CNetConfig();
    void print()const;
    void grab();
    void drop();
    bool check();

private:
    s32 mReference;
};


} // end namespace net
} // end namespace irr


#endif //APP_HNETCONFIG_H
