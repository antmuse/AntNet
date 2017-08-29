#ifndef APP_HNETCONFIG_H
#define APP_HNETCONFIG_H

#include "HConfig.h"
#include "irrTypes.h"


#if defined(APP_PLATFORM_WINDOWS)
#if defined(APP_OS_64BIT)
typedef irr::u64 netsocket;
#else
typedef irr::u32 netsocket;
#endif
#define APP_INVALID_SOCKET  (~0)
#define APP_SOCKET_ERROR     (-1)
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
typedef irr::s32 netsocket;
#define APP_INVALID_SOCKET  (-1)
#define APP_SOCKET_ERROR     (-1)
#endif



///Socket IO cache size is 8K (8192=1024*8)
#define APP_NET_MAX_BUFFER_LEN     8192
#define APP_NET_DEFAULT_PORT           9011
#define APP_NET_DEFAULT_IP                ("127.0.0.1")
#define APP_NET_LOCAL_HOST              ("localhost")

///Server default threads count
#define APP_NET_DEFAULT_SERVER_THREAD   2

///The tick time, we need keep the net link alive.
#define APP_NET_TICK_TIME  (30*1000)

///We will relink the socket if tick count >= APP_NET_TICK_MAX_COUNT
#define APP_NET_TICK_MAX_COUNT  (4)



namespace irr {
    namespace net {

        enum ENetMessage {
            ENET_NULL = 0,
            ENET_ERROR,
            ENET_WAIT,
            ENET_HI,
            ENET_BYE,
            ENET_DATA,
            ENET_LINK,
            ENET_RESET,
            ENET_CLEAR,
            ENET_COUNT
        };

        const c8* const AppNetMessageNames[] = {
            "NetInit",
            "NetError",
            "NetWait",
            "NetHi",
            "NetBye",
            "NetData",
            "NetLink",
            "NetReset",
            "NetClear",
            0
        };



        enum ENetNodeType {
            ENET_UNKNOWN = 0,
            ENET_TCP_CLIENT,
            ENET_UDP_CLIENT,
            ENET_TCP_SERVER_NORMALL,
            ENET_UDP_SERVER_NORMALL,
            ENET_TCP_SERVER_SENIOR,
            ENET_UDP_SERVER_SENIOR,
            ENET_NODE_TYPE_COUNT
        };

        const c8* const AppNetNodeNames[] = {
            "Unknown",
            "ClientTCP",
            "ClientUDP",
            "ServerNormalTCP",
            "ServerNormalUDP",
            "ServerSeniorTCP",
            "ServerSeniorUDP",
            0
        };


    } // end namespace net
} // end namespace irr


#endif //APP_HNETCONFIG_H
