#ifndef APP_INETEVENTERHTTP_H
#define APP_INETEVENTERHTTP_H


#include "INetEventer.h"

namespace irr {
namespace net {
class CNetHttpResponse;
//class CNetHttpRequest;
//class CNetPacket;

class INetEventerHttp {
public:
    INetEventerHttp() {
    }

    virtual ~INetEventerHttp() {
    }

    /**
    *@brief Callback function when received net packet.
    *@param The received package.
    */
    virtual void onReceive(const CNetHttpResponse& it) = 0;

    /**
    *@brief Called when net status changed.
    *@param iEvent The event type.
    */
    virtual s32 onEvent(SNetEvent& iEvent) = 0;
};


}// end namespace net
}// end namespace irr

#endif //APP_INETEVENTERHTTP_H
