#ifndef APP_CDEFAULTNETEVENTER_H
#define APP_CDEFAULTNETEVENTER_H

#include "INetEventer.h"

namespace irr {
namespace net {
class INetSession;
class CNetServiceTCP;
class CNetServerAcceptor;


//for net test
class CDefaultNetEventer : public INetEventer {
public:
    CDefaultNetEventer();

    void setAutoConnect(bool autoconnect) {
        mAutoConnect = autoconnect;
    }

    void setHub(CNetServiceTCP* hub) {
        mHub = hub;
    }

    void setServer(CNetServerAcceptor* hub) {
        mServer = hub;
    }

    void setSession(u32 it) {
        mSession = it;
    }

    virtual ~CDefaultNetEventer();

    virtual s32 onConnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

private:
    bool mAutoConnect;
    u32 mSession;
    CNetServiceTCP* mHub;
    CNetServerAcceptor* mServer;
    CNetPacket mPacket;
};

}//namespace net
}//namespace irr

#endif //APP_CDEFAULTNETEVENTER_H