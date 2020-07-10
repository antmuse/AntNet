#ifndef APP_CTLSCONTEXT_H
#define APP_CTLSCONTEXT_H

#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include "CString.h"
#include "CNetPacket.h"
#include "CNetCert.h"
#include "CNetService.h"
#include "INetEventer.h"

namespace app {
namespace net {

class CTlsContext : public INetEventer {
public:
    CTlsContext();

    ~CTlsContext();

    virtual INetEventer* onAccept(const CNetAddress& local)override;

    virtual s32 onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote)override;

    void setEventer(INetEventer* val) {
        mEvent = val;
    }

    INetEventer* getEventer()const {
        return mEvent;
    }

    void setHubHost(CNetServiceTCP* hub, const s8* iHost) {
        mHub = hub;
        mHost = iHost;
    }

    CNetCert& getCert() {
        return mCertCA;
    }

    const CNetCert& getCert()const {
        return mCertCA;
    }

    bool getHandshake()const {
        return mHandshake;
    }

    //上层调用
    s32 send(const void* buf, usz iSize);


    //内部调用
    CNetPacket& getCache() {
        return mPacket;
    }

    //内部调用
    s32 writeRaw(const void* buf, s32 iSize);

private:
    CTlsContext(const CTlsContext&) = delete;
    CTlsContext(const CTlsContext&&) = delete;
    CTlsContext& operator=(const CTlsContext&) = delete;
    CTlsContext& operator=(const CTlsContext&&) = delete;

    /**
    * @param currSend current status: true=send or false=recieve
    *        eg: on connect event, handshake(true);
    * @return 0=success, <0=continue, 1=send, 2=receive
    */
    s32 handshake(bool currSend);

    /**
    * @param perName just for debug name
    */
    bool init(bool iClient, const s8* iHost, const s8* perName = nullptr);


    bool mHandshake;
    bool mTypeClient;
    u32 mID;
    core::CString mHost;
    INetEventer* mEvent;
    CNetServiceTCP* mHub;
    CNetAddress mLocal;
    CNetPacket mPacket; //缓存未解密数据
    CNetCert mCertCA;
    mbedtls_entropy_context mEntropy;
    mbedtls_ctr_drbg_context mDebug;
    mbedtls_ssl_context mSSL;
    mbedtls_ssl_config mConfig;
};

}//namespace net
}//namespace app

#endif //APP_CTLSCONTEXT_H