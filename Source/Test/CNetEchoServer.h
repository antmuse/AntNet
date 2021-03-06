#ifndef APP_CNETECHOSERVER_H
#define APP_CNETECHOSERVER_H

#include "CNetEchoClient.h"

namespace app {
namespace net {
class CNetServerAcceptor;


//echo server
class CNetEchoServer : public INetEventer {
public:
    CNetEchoServer();

    virtual ~CNetEchoServer();

    void setServer(CNetServerAcceptor* hub);

    virtual INetEventer* onAccept(const CNetAddress& local)override {
        return this;
    }

    virtual s32 onTimeout(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onConnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    u32 getLinkCount() const {
        return mLinkCount;
    }
    u32 getDislinkCount() const {
        return mDislinkCount;
    }
    u32 getSentSuccessSize()const {
        return mSendSuccessBytes;
    }
    u32 getSentFailSize()const {
        return mSendFailBytes;
    }
    u32 getRecvSize() const {
        return mRecvBytes;
    }
    u32 getSendSize() const {
        return mSendBytes;
    }
    
private:
    s32 mLinkCount;
    s32 mDislinkCount;
    s32 mSendSuccessBytes;
    s32 mSendFailBytes;
    s32 mRecvBytes;
    s32 mSendBytes;
    CNetServerAcceptor* mServer;
};

}//namespace net
}//namespace app

#endif //APP_CNETECHOSERVER_H