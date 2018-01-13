#ifndef APP_INETSERVER_H
#define APP_INETSERVER_H

#include "HNetConfig.h"
#include "INetEventer.h"
#include "IPairOrderID.h"
//#include "SNetAddress.h"

namespace irr {
namespace net {

///a client id pair(IP, Port)
typedef IPairOrderID<u32, u16> CNetClientID;

struct SNetAddress;



class INetServer {
public:
    INetServer() {
    }

    virtual ~INetServer() {
    }

    virtual ENetNodeType getType() const = 0;

    virtual void setNetEventer(INetEventer* it) = 0;

    virtual bool start() = 0;

    virtual bool stop() = 0;

    virtual void setLocalAddress(const SNetAddress& it) = 0;

    virtual const SNetAddress& getLocalAddress() const = 0;

    virtual void setMaxClients(u32 max) = 0;

    virtual u32 getClientCount() const = 0;
};

}// end namespace net
}// end namespace irr

#endif //APP_INETSERVER_H
