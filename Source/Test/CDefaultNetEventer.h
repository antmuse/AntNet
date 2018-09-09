#ifndef APP_CDEFAULTNETEVENTER_H
#define APP_CDEFAULTNETEVENTER_H

#include "INetEventer.h"

namespace irr {
namespace net {
class INetSession;
class INetClientSeniorTCP;
class CNetServerAcceptor;


//for net test
class CDefaultNetEventer : public INetEventer {
public:
    CDefaultNetEventer();

    void setAutoConnect(bool autoconnect) {
        mAutoConnect = autoconnect;
    }

    void setHub(INetClientSeniorTCP* hub) {
        mHub = hub;
    }

    void setServer(CNetServerAcceptor* hub) {
        mServer = hub;
    }

    void setSession(u32 it) {
        mSession = it;
    }

    virtual ~CDefaultNetEventer();

    virtual s32 onEvent(SNetEvent& iEvent)override;

private:
    bool mAutoConnect;
    u32 mSession;
    INetClientSeniorTCP* mHub;
    CNetServerAcceptor* mServer;
};

}//namespace net
}//namespace irr

#endif //APP_CDEFAULTNETEVENTER_H