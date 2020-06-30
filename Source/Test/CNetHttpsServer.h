#ifndef APP_CNETHTTPSSERVER_H
#define APP_CNETHTTPSSERVER_H


#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "CNetCert.h"

#include "AppList.h"
#include "INetEventer.h"
#include "CNetServerAcceptor.h"

namespace app {
namespace net {
class CNetHttpsServer;

class CNetHttpsNode : public INetEventer {
public:
    CNetHttpsNode(CNetHttpsServer* hub);
    ~CNetHttpsNode();

    void grab();

    bool drop();

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

    s32 sendBuffer(const void* buf, s32 len);

    CNetPacket& getReadCache() {
        return mPacket;
    }

private:

    /**
    * @param currSend current status: true=send or false=recieve
    *        eg: on connect event, handshake(true);
    * @return 0=success, <0=continue, 1=send, 2=receive
    */
    s32 handshake(bool currSend);

    s32 getStateOfTLS()const {
        return mSSL.state;
    }

    volatile u8 mStatus;  //0=disconnected, 1=connecting, 2=connected
    bool mHandshake;
    s32 mUseCount;        //grap/drop
    u32 mConnetID;
    CNetPacket mPacket;
    mbedtls_ssl_context mSSL;
    CNetHttpsServer* mHttpsHub;
};



//echo server
class CNetHttpsServer : public INetEventer {
public:
    CNetHttpsServer();

    virtual ~CNetHttpsServer();

    CNetServiceTCP* getServer() {
        return &mServer;
    }

    virtual INetEventer* onAccept(const CNetAddress& local)override;

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

    mbedtls_ssl_config& getTlsConfig() {
        return conf;
    }

    bool start(u16 listenPort);

    bool stop();

    void show();

private:
    s32 mLinkCount;
    s32 mDislinkCount;
    s32 mSendSuccessBytes;
    s32 mSendFailBytes;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_pk_context pkey;
    CNetCert mCertCA;
    net::CNetConfig mConfig;
    net::CNetServerAcceptor mLisenServer;
    net::CNetServiceTCP mServer;
};

}//namespace net
}//namespace app

#endif //APP_CNETHTTPSSERVER_H