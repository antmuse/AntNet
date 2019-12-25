#ifndef APP_HNETCONFIG_H
#define APP_HNETCONFIG_H

#include "HConfig.h"
#include "irrTypes.h"


namespace irr {
namespace net {

//#define APP_NET_USE_IPV6


///Socket IO cache size is 8K (8192=1024*8)
#define APP_NET_MAX_BUFFER_LEN     8192
#define APP_NET_DEFAULT_PORT       9981
#define APP_NET_DEFAULT_IP         ("127.0.0.1")
#define APP_NET_ANY_IP             ("0.0.0.0")
#define APP_NET_ANY_PORT           (0)
#define APP_NET_LOCAL_HOST         ("localhost")



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

    //max worker's threads, range[1-255]
    u8 mMaxWorkThread;
    //range[8,64]
    u8 mMaxPostAccept;
    //range[32,512]
    u16 mMaxFetchEvents;

    u32 mLinger;
    u32 mKeepAliveIdle;
    u32 mKeepAliveInterval;
    u32 mKeepAliveMaxTick;
    u32 mSendTimeout;
    u32 mReceiveTimeout;
    u32 mSendCacheSize;
    u32 mRecieveCacheSize;

    //server
    u32 mMaxContext;        ///<max session on each epoll thread
    u32 mMaxSpeed;          ///<max speed of server, bytes/second
    //range[10,60000]
    u32 mPollTimeout; ///<in ms
    //range[1000,60000]
    u32 mSessionTimeout; ///<in ms

    CNetConfig();

    void print()const;

    // thread safe
    void grab();

    // thread safe
    void drop();

    bool check();

private:
    s32 mReference;
};


} // end namespace net
} // end namespace irr


#endif //APP_HNETCONFIG_H
