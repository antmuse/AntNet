#ifndef APP_CNETSERVERTCP_H
#define APP_CNETSERVERTCP_H

#include "INetServer.h"
#include "CNetPacket.h"
#include "IRunnable.h"
#include "CNetSocket.h"
#include "CStreamFile.h"

namespace irr {
class CThread;

namespace net {


/**
* @brief This is a simple server.
*/
class CNetServerTCP : public INetServer, public IRunnable {
public:
    CNetServerTCP();
    virtual ~CNetServerTCP();

    virtual ENetNodeType getType() const override {
        return ENET_TCP_SERVER_LOW;
    }

    virtual void setNetEventer(INetEventer* it) override {
        mReceiver = it;
    }

    virtual void run()override;

    virtual bool start()override;

    virtual bool stop()override;

    virtual void setLocalAddress(const SNetAddress& it)override {
        mAddressLocal = it;
    }

    virtual const SNetAddress& getLocalAddress() const override {
        return mAddressLocal;
    }

    virtual s32 sendData(const c8* iData, s32 iLength);


    virtual void setMaxClients(u32 max) override {

    }

    virtual u32 getClientCount() const override {
        return mClientCount;
    }


private:
    bool mRunning;
    u8 mStatus;
    u8 mClientCount;
    INetEventer* mReceiver;
    CThread* mThread;
    io::CStreamFile mStream;
    CNetPacket mPacket;
    CNetSocket mSocket;
    CNetSocket mSocketSub;
    SNetAddress mAddressRemote;
    SNetAddress mAddressLocal;
};

}// end namespace net
}// end namespace irr

#endif //APP_CNETSERVERTCP_H
