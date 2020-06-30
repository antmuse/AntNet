#ifndef APP_CNETSERVERTCP_H
#define APP_CNETSERVERTCP_H

#include "INetServer.h"
#include "CNetPacket.h"
#include "IRunnable.h"
#include "CNetSocket.h"
#include "CNetAddress.h"

namespace app {
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

    virtual void setLocalAddress(const CNetAddress& it)override {
        mAddressLocal = it;
    }

    virtual const CNetAddress& getLocalAddress() const override {
        return mAddressLocal;
    }

    virtual s32 send(const s8* iData, s32 iLength);


    virtual void setMaxClients(u32 max) override {

    }

    virtual u32 getClientCount() const override {
        return mClientCount;
    }


private:
    u8 mClientCount;
    volatile s32 mRunning;
    u32 mStatus;
    INetEventer* mReceiver;
    INetEventer* mReceiver2;
    CThread* mThread;
    CNetPacket mPacket;
    CNetSocket mSocket;
    CNetSocket mSocketSub;
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;

    bool clearError();
};

}// end namespace net
}// end namespace app

#endif //APP_CNETSERVERTCP_H
