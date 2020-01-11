#ifndef APP_CNETCLIENTNATPUNCHER_H
#define APP_CNETCLIENTNATPUNCHER_H

#include "INetClient.h"
#include "IRunnable.h"
#include "irrString.h"
#include "CNetProtocal.h"
#include "CNetPacket.h"
#include "CNetSocket.h"


namespace irr {
namespace net {


class CNetClientNatPuncher : public INetClient, public INetDataSender {
public:

    CNetClientNatPuncher();

    virtual ~CNetClientNatPuncher();


    virtual ENetNodeType getType() const override {
        return ENET_UDP_CLIENT;
    }


    virtual void setNetEventer(INetEventer* it)override {
        mReceiver = it;
    }

    bool update(s64 iTime)override;

    virtual bool start(bool useThread)override;

    virtual bool stop()override;

    virtual s32 send(const void* iData, s32 iLength) override;

    virtual void setLocalAddress(const CNetAddress& it)override {
        mAddressLocal = it;
    }

    virtual void setRemoteAddress(const CNetAddress& it)override {
        mAddressServer = it;
    }

    virtual const CNetAddress& getRemoteAddress() const override {
        return mAddressPeer;
    }

    virtual const CNetAddress& getLocalAddress() const override {
        return mAddressLocal;
    }

    void setID(u32 id);


    u32 getID() const {
        return mProtocal.getID();
    }

    s32 sendBuffer(void* iUserPointer, const c8* iData, s32 iLength)override;


private:
    bool clearError();

    void resetSocket();

    void sendTick();

    void sendBye();

    APP_INLINE void onPacket(CNetPacket& it);
    /**
    *@param iStepTime relative time step
    */
    void step(u64 iStepTime);

    bool bindLocal();

    bool mRunning;
    s8 mPunchCount;
    u32 mStatus;
    u32 mTickCount;
    u64 mUpdateTime;
    u64 mTickTime;
    INetEventer* mReceiver;
    CNetProtocal mProtocal;
    CNetPacket mPacket;
    CNetAddress mAddressRemote;
    CNetAddress mAddressServer;
    CNetAddress mAddressPeer;
    CNetAddress mAddressLocal;
    CNetSocket mConnector;
};


}// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTNATPUNCHER_H
