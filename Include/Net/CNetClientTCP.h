#ifndef APP_CNETCLIENTTCP_H
#define APP_CNETCLIENTTCP_H

#include "INetClient.h"
#include "irrString.h"
#include "CNetSocket.h"
#include "CNetPacket.h"
#include "CThread.h"

namespace irr {
namespace net {


class CNetClientTCP : public INetClient, public IRunnable {
public:
    CNetClientTCP();

    virtual ~CNetClientTCP();

    virtual void run() override;

    virtual ENetNodeType getType() const override {
        return ENET_TCP_CLIENT;
    }

    virtual void setNetEventer(INetEventer* it) override {
        mReceiver = it;
    }

    virtual bool update(s64 iTime)override;

    virtual bool start(bool useThread)override;

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

    virtual s32 send(const void* iData, s32 iLength)override;

    const CNetSocket& getSocket()const {
        return mConnector;
    }

private:
    void resetSocket();
    bool connect();

    /**
    *@return True if not really error, else false.
    */
    bool clearError();

    volatile s32 mRunning;
    u32 mStatus;
    s64 mLastUpdateTime;    ///<last absolute time of update
    INetEventer* mReceiver;
    CThread* mThread;       //receive thread
    CNetPacket mPacket;
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;
    CNetSocket mConnector;
};

}// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTTCP_H
