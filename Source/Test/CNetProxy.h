#ifndef APP_CNETPROXY_H
#define APP_CNETPROXY_H

#include "CNetServerAcceptor.h"

namespace irr {
namespace net {
class CNetProxy;


class CNetProxyNode : public INetEventer {
public:
    CNetProxyNode(CNetProxy* hub, bool front) :
        mFrontNet(front), mConnetID(0), mStatus(0),
        mPairNode(nullptr), mProxyHub(hub), mUseCount(1) {
    }

    void grab();

    bool drop();

    void setPairNode(CNetProxyNode* its) {
        if (mPairNode) {
            mPairNode->drop();
        }
        if (its) {
            its->grab();
        }
        mPairNode = its;
    }

    virtual INetEventer* onAccept(const CNetAddress& local)override {
        APP_ASSERT(0);
        return nullptr;
    }

    virtual s32 onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

private:
    bool mFrontNet;       //front sock=true, backend sock=false;
    volatile u8 mStatus;  //0=disconnected, 1=connecting, 2=connected
    s32 mUseCount;        //grap/drop
    u32 mConnetID;
    CNetProxyNode* mPairNode;
    CNetProxy* mProxyHub;
};








/*
* @class CNetProxy
* usage:
* void AppRunProxy() {
*     net::CNetProxy sev;
*     sev.setProxyHost("127.0.0.1:60000");
*     sev.start(60001);
*     sev.show();
*     sev.stop();
* }
*/
class CNetProxy : public INetEventer {
public:
    CNetProxy();

    virtual ~CNetProxy();

    virtual INetEventer* onAccept(const CNetAddress& local)override;

    virtual s32 onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    /**
    * @param host
    * eg: setProxyHost("127.0.0.1:60000");
    */
    void setProxyHost(const c8* host);

    bool start(u16 listenPort);

    bool stop();

    void show();

    net::CNetServiceTCP& getServer() {
        return mServer;
    }

    net::CNetAddress& getProxyAddress() {
        return mRemoteAddr;
    }

private:
    s32 mLinkCount;
    s32 mDislinkCount;
    s32 mSendSuccessBytes;
    s32 mSendFailBytes;
    net::CNetConfig mConfig;
    net::CNetServerAcceptor mLisenServer;
    net::CNetServiceTCP mServer;
    net::CNetAddress mRemoteAddr;
};

}//namespace net
}//namespace irr

#endif //APP_CNETPROXY_H