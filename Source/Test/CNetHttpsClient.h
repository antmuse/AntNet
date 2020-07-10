#ifndef APP_CNETHTTPSCLIENT_H
#define APP_CNETHTTPSCLIENT_H

#include "CTlsContext.h"
#include "AppList.h"
#include "INetEventer.h"

namespace app {
namespace net {
class CNetServiceTCP;

//echo client
class CNetHttpsClient : public INetEventer {
public:
    CNetHttpsClient();

    virtual ~CNetHttpsClient();

    INetEventer* getTlsEvent() {
        return &mTlsCtx;
    }

    void setHubHost(CNetServiceTCP* hub, const s8* iHost) {
        mTlsCtx.setHubHost(hub, iHost);
    }

    virtual INetEventer* onAccept(const CNetAddress& local)override {
        return nullptr;
    }

    virtual s32 onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    s32 send(const void* buf, s32 len);

private:
    CNetPacket mPacket;
    CTlsContext mTlsCtx;
};

}//namespace net
}//namespace app

#endif //APP_CNETHTTPSCLIENT_H