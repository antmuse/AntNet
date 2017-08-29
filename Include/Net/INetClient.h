#ifndef APP_INETCLIENT_H
#define APP_INETCLIENT_H

#include "HNetConfig.h"
#include "INetEventer.h"

namespace irr {
namespace net {

class INetClient {
public:
	INetClient(){
	}
	virtual ~INetClient(){
	}

    /**
    *@brief update net connection
    *@param iTime absolute time in millisecond.
    *@return false if connection dead, else true.
    */
    virtual bool update(u32 iTime) = 0;

    virtual ENetNodeType getType() const = 0;

    virtual void setNetEventer(INetEventer* it) = 0;

	virtual bool start() = 0;

	virtual bool stop() = 0;

	virtual void setIP(const c8* ip) = 0;

	virtual void setPort(u16 port) = 0;

	virtual const c8* getIP() const = 0;

	virtual u16 getPort() const = 0;

    virtual const c8* getLocalIP() const = 0;

    virtual u16 getLocalPort() const = 0;

	virtual s32 sendData(const c8* iData, s32 iLength) = 0;

	virtual s32 sendData(const net::CNetPacket& iData) = 0;
};

}// end namespace net
}// end namespace irr

#endif //APP_INETCLIENT_H
