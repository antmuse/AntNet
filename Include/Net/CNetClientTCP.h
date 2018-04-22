#ifndef APP_CNETCLIENTTCP_H
#define APP_CNETCLIENTTCP_H

#include "INetClient.h"
#include "irrString.h"
#include "CStreamFile.h"
#include "CNetSocket.h"
#include "CNetPacket.h"


namespace irr {
namespace net {


class CNetClientTCP : public INetClient {
public:
    CNetClientTCP();

    virtual ~CNetClientTCP();

    virtual ENetNodeType getType() const override {
        return ENET_TCP_CLIENT;
    }

    virtual void setNetEventer(INetEventer* it) override {
        mReceiver = it;
    }

    virtual bool update(u64 iTime)override;

    virtual bool start()override;

    virtual bool stop()override;

    virtual void setLocalAddress(const CNetAddress& it)override {
        mAddressLocal = it;
    }

    virtual void setRemoteAddress(const CNetAddress& it)override {
        mAddressRemote = it;
    }

    virtual const CNetAddress& getRemoteAddress() const override {
        return mAddressRemote;
    }

    virtual const CNetAddress& getLocalAddress() const override {
        return mAddressLocal;
    }

    virtual s32 sendData(const c8* iData, s32 iLength)override;

private:
    void resetSocket();
    void connect();
    void sendTick();

    /**
    *@return True if not really error, else false.
    */
    bool clearError();

    /**
    *@return True if a package had been sent completely, else false.
    *@note Can't write other data into socket cache if return false.
    */
    bool flushDatalist();

    void setNetPackage(ENetMessage it, net::CNetPacket& out);


    bool mRunning;
    u8 mStatus;
    u32 mTickCount;         ///<Heartbeat count
    u32 mPacketSize;
    u64 mTickTime;          ///<Heartbeat time
    u64 mLastUpdateTime;    ///<last absolute time of update
    INetEventer* mReceiver;
    CNetPacket mPacket;
    io::CStreamFile mStream;
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;
    CNetSocket mConnector;
};

}// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTTCP_H
