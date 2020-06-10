#ifndef APP_CNETHTTPSCLIENT_H
#define APP_CNETHTTPSCLIENT_H


#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "CNetCert.h"

#include "irrList.h"
#include "INetEventer.h"

namespace irr {
namespace net {
class CNetServiceTCP;

//echo client
class CNetHttpsClient : public INetEventer {
public:
    CNetHttpsClient();

    virtual ~CNetHttpsClient();

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

    s32 sendBuffer(const void* buf, s32 len);

    CNetPacket& getReadCache() {
        return mPacket;
    }
    s32 getStateOfTLS()const {
        return ssl.state;
    }
private:
    //c8 mTestData[4 * 1024];
    bool mAutoConnect;
    bool mHandshake;
    u32 mSession;
    CNetServiceTCP* mHub;
    CNetPacket mPacket;

    //mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    CNetCert mCertCA;

    /**
    * @param currSend current status: true=send or false=recieve
    *        eg: on connect event, handshake(true);
    * @return 0=success, <0=continue, 1=send, 2=receive
    */
    s32 handshake(bool currSend);
};

}//namespace net
}//namespace irr

#endif //APP_CNETHTTPSCLIENT_H