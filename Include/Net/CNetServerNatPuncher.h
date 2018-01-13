#ifndef APP_CNETSERVERNATPUNCHER_H
#define APP_CNETSERVERNATPUNCHER_H

#include "irrMap.h"
#include "INetServer.h"
#include "IRunnable.h"
#include "CNetSocket.h"

namespace irr {
class CThread;

namespace net {

class CNetServerNatPuncher : public IRunnable {
public:
    CNetServerNatPuncher();

    virtual ~CNetServerNatPuncher();

    virtual void run()override;

    bool start();

    bool stop();

    void setLocalAddress(const SNetAddress& it) {
        mAddressLocal = it;
    }

    const SNetAddress& getLocalAddress() const {
        return mAddressLocal;
    }

private:
    struct SClientNode {
        u64 mTime;
        //u32 mLocalIP;
        //u16 mLocalPort;
        SNetAddress mAddress;
    };

    const core::map<CNetClientID, SClientNode*>::Node* getAnyRemote(CNetClientID& my)const;
    const core::map<CNetClientID, SClientNode*>::Node* getRemote(CNetClientID& my)const;
    void checkTimeout();
    void removeAllClient();
    bool initialize();
    void removeAll();
    bool clearError();

    bool mRunning;                  ///<True if server started, else false
    u32 mOverTimeInterval;
    u64 mCurrentTime;
    core::map<CNetClientID, SClientNode*>  mAllClient;
    CNetSocket mConnector;
    SNetAddress mAddressRemote;
    SNetAddress mAddressLocal;
    CThread* mThread;
};


} //namespace net
} //namespace irr

#endif //APP_CNETSERVERNATPUNCHER_H
