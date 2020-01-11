#ifndef APP_INETCLIENT_H
#define APP_INETCLIENT_H

#include "INetEventer.h"

namespace irr {
namespace net {

class CNetAddress;

class INetClient {
public:
    INetClient() {
    }
    virtual ~INetClient() {
    }

    /**
    *@brief update net connection
    *@param iTime absolute time in millisecond.
    *@return false if connection dead, else true.
    */
    virtual bool update(s64 iTime) = 0;

    virtual ENetNodeType getType() const = 0;

    virtual void setNetEventer(INetEventer* it) = 0;

    virtual bool start(bool useThread) = 0;

    virtual bool stop() = 0;

    virtual void setLocalAddress(const CNetAddress& it) = 0;

    virtual void setRemoteAddress(const CNetAddress& it) = 0;

    virtual const CNetAddress& getRemoteAddress() const = 0;

    virtual const CNetAddress& getLocalAddress() const = 0;

    virtual s32 send(const void* iData, s32 iLength) = 0;

};

}// end namespace net
}// end namespace irr

#endif //APP_INETCLIENT_H
