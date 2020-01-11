#ifndef APP_INETSERVER_H
#define APP_INETSERVER_H

#include "INetEventer.h"

namespace irr {
namespace net {


class CNetAddress;

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

    virtual void setLocalAddress(const CNetAddress& it) = 0;

    virtual const CNetAddress& getLocalAddress() const = 0;

    virtual void setMaxClients(u32 max) = 0;

    virtual u32 getClientCount() const = 0;
};

}// end namespace net
}// end namespace irr

#endif //APP_INETSERVER_H
