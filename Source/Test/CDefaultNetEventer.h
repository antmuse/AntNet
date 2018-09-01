#ifndef APP_CDEFAULTNETEVENTER_H
#define APP_CDEFAULTNETEVENTER_H

#include "INetEventer.h"

namespace irr {
namespace net {
class INetSession;
class INetClientSeniorTCP;

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

    void setSession(INetSession* it) {
        mSession = it;
    }

    virtual ~CDefaultNetEventer();

    virtual s32 onEvent(SNetEvent& iEvent)override;

private:
    bool mAutoConnect;
    INetSession* mSession;
    INetClientSeniorTCP* mHub;
};

}//namespace net
}//namespace irr

#endif //APP_CDEFAULTNETEVENTER_H