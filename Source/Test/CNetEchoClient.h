#ifndef APP_CNETECHOCLIENT_H
#define APP_CNETECHOCLIENT_H

#include "AppList.h"
#include "INetEventer.h"

namespace app {
namespace net {
class CNetServiceTCP;

//echo client
class CNetEchoClient : public INetEventer {
public:
    CNetEchoClient();

    virtual ~CNetEchoClient();

    void setAutoConnect(bool autoconnect) {
        mAutoConnect = autoconnect;
    }

    void setHub(CNetServiceTCP* hub) {
        mHub = hub;
    }

    void setSession(u32 it) {
        mSession = it;
    }

    virtual INetEventer* onAccept(const CNetAddress& local)override {
        return NULL;
    }

    virtual s32 onConnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onTimeout(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    static s32 mSendRequest;
    static s32 mSendRequestFail;
    static s32 mSendSuccess;
    static s32 mSendFail;
    static s32 mRecvCount;
    static s32 mTickRequest;
    static s32 mTickRequestFail;
    static s32 mTickRecv;
    static s32 mSendSuccessBytes;
    static s32 mSendFailBytes;
    static s32 mRecvBytes;
    static s32 mRecvBadCount;
    static s32 mMaxSendPackets;

    static void initData(const s8* msg, s32 mb);

private:
    static s8 mTestData[4 * 1024];

    bool mAutoConnect;
    u32 mSession;
    CNetServiceTCP* mHub;
    CNetPacket mPacket;

    void send();
    void sendTick();
};

}//namespace net
}//namespace app

#endif //APP_CNETECHOCLIENT_H