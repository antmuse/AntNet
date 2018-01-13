/**
*@file CNetServerSeniorUDP.h
*Defined CNetServerSeniorUDP, SContextIO, SClientContextUDP
*/

#ifndef APP_CNETSERVERSENIORUDP_H
#define APP_CNETSERVERSENIORUDP_H


#include "INetServer.h"
#include "irrString.h"
#include "irrMap.h"
#include "IRunnable.h"
#include "SClientContext.h"
#include "CThread.h"
#include "CNetSocket.h"
#include "CNetProtocal.h"



namespace irr {
namespace net {
/**
*@class CNetServerSeniorUDP
*/
class CNetServerSeniorUDP : public INetServer, public IRunnable, public INetDataSender {
public:
    CNetServerSeniorUDP();

    virtual ~CNetServerSeniorUDP();

    virtual ENetNodeType getType() const {
        return ENET_UDP_SERVER;
    }

    virtual void setNetEventer(INetEventer* it) {
        mReceiver = it;
    }

    virtual s32 sendBuffer(void* iUserPointer, const c8* iData, s32 iLength)override;

    virtual void run();

    virtual bool start();

    virtual bool stop();

    virtual void setLocalAddress(const SNetAddress& it)override {
        mAddressLocal = it;
    }

    virtual const SNetAddress& getLocalAddress() const override {
        return mAddressLocal;
    }

    virtual void setMaxClients(u32 max) {
        mMaxClientCount = max;
    };

    virtual u32 getClientCount() const {
        return mAllClient.size();
    }

    /**
    *@brief Request a receive action.
    *@param iContext,
    *@return True if request finished, else false.
    */
    bool stepReceive(SClientContextUDP* iContext);


    void addClient(SClientContextUDP* iContext);


    void removeClient(CNetClientID id);


    bool clearError();

    virtual void setThreads(u32 max) {
    }

protected:
    void removeOvertimeClient();
    bool initialize();
    void removeAll();
    void removeAllClient();
    APP_INLINE void onPacket(CNetPacket& it);

private:
    bool mRunning;                                       ///<True if server started, else false
    u32 mMaxClientCount;
    u64 mCurrentTime;
    u64 mOverTimeInterval;
    core::map<CNetClientID, SClientContextUDP*>  mAllClient;          ///<All the clients's socket context
    CThread* mThread;								                         ///<busy worker
    INetEventer* mReceiver;
    SNetAddress mAddressRemote;
    SNetAddress mAddressLocal;
    CNetSocket mConnector;
    //CMemoryHub mMemHub;
};


}// end namespace net
}// end namespace irr

#endif //APP_CNETSERVERSENIORUDP_H
