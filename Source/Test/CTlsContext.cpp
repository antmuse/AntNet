#include "CTlsContext.h"
#include "CLogger.h"


namespace app {
namespace net {

static s32 AppTlsSend(void* ctx, const u8* buf, usz len) {
    CTlsContext* nd = reinterpret_cast<CTlsContext*>(ctx);
    return nd->writeRaw(buf, static_cast<s32>(len));
}

static s32 AppTlsRecieve(void* ctx, u8* buf, usz len) {
    CTlsContext* nd = reinterpret_cast<CTlsContext*>(ctx);
    CNetPacket& pk = nd->getCache();
    u32 red = pk.getSize();
    if(red == 0) {
        return 0;
    }
    if(red > len) { red = static_cast<u32>(len); }
    memcpy(buf, pk.getConstPointer(), red);
    pk.clear(red);
    return red;
}

static void AppTlsDebug(void* ctx, s32 level, const s8* file, s32 line, const s8* str) {
    ((void)level);
    printf("%s:%04d: %s", file, line, str);
}



CTlsContext::CTlsContext() : mHub(nullptr), mID(0), mHandshake(true), mEvent(nullptr), mTypeClient(true) {
    mbedtls_ssl_init(&mSSL);
    mbedtls_ssl_config_init(&mConfig);
    mbedtls_ctr_drbg_init(&mDebug);
    mbedtls_entropy_init(&mEntropy);
    mbedtls_ssl_set_bio(&mSSL, this, AppTlsSend, AppTlsRecieve, NULL);
}

bool CTlsContext::init(bool iClient, const s8* iHost, const s8* perName) {
    const s8* pers = nullptr == perName ? "CTlsContext" : perName;
    s32 ret = 0;
    if((ret = mbedtls_ctr_drbg_seed(&mDebug, mbedtls_entropy_func, &mEntropy, (const u8*)pers, strlen(pers))) != 0) {
        CLogger::logError("CTlsContext::init", "mbedtls_ctr_drbg_seed returned %d\n", ret);
        APP_ASSERT(0);
        return false;
    }

    //if(!mCertCA.loadFromFile("D:/App/SSH/client.cer"))
    if(!mCertCA.loadFromBuf(nullptr, 0)) {
        CLogger::logError("CTlsContext::init", "mbedtls_x509_crt_parse fail");
        APP_ASSERT(0);
        return false;
    }

    if((ret = mbedtls_ssl_config_defaults(&mConfig,
        iClient ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        CLogger::logError("CTlsContext::init", "mbedtls_ssl_config_defaults returned %d", ret);
        APP_ASSERT(0);
        return false;
    }

    /* OPTIONAL is not optimal for security,
    * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&mConfig, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&mConfig, &mCertCA.getCert(), nullptr);
    mbedtls_ssl_conf_rng(&mConfig, mbedtls_ctr_drbg_random, &mDebug);
    mbedtls_ssl_conf_dbg(&mConfig, AppTlsDebug, stdout);

    if((ret = mbedtls_ssl_setup(&mSSL, &mConfig)) != 0) {
        CLogger::logError("CTlsContext::init", "mbedtls_ssl_setup returned %d", ret);
        APP_ASSERT(0);
        return false;
    }
    if((ret = mbedtls_ssl_set_hostname(&mSSL, iHost)) != 0) {
        CLogger::logError("CTlsContext::init", "mbedtls_ssl_set_hostname returned %d", ret);
        APP_ASSERT(0);
        return false;
    }
    return true;
}


CTlsContext::~CTlsContext() {
    mbedtls_ssl_close_notify(&mSSL);
    mbedtls_ssl_free(&mSSL);
    mbedtls_ssl_config_free(&mConfig);
    mbedtls_ctr_drbg_free(&mDebug);
    mbedtls_entropy_free(&mEntropy);
}


s32 CTlsContext::send(const void* buf, usz iSize) {
    s32 ret = mbedtls_ssl_write(&mSSL, (const u8*)buf, iSize);
    return ret;
}


s32 CTlsContext::writeRaw(const void* buf, s32 iSize) {
    APP_ASSERT(mHub);
    return mHub->send(mID, buf, iSize);
}


s32 CTlsContext::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    mHandshake = true;
    mPacket.setUsed(0);
    mID = sessionID;
    mLocal = local;
    mTypeClient = false;
    init(mTypeClient, mHost.c_str(), "tls-server");
    CLogger::log(ELOG_INFO, "CTlsContext::onLink", "[%u,%s:%u->%s:%u]",
        sessionID, local.getIPString(), local.getPort(), remote.getIPString(), remote.getPort());
    APP_ASSERT(mEvent);
    return 0;// mEvent->onLink(sessionID, local, remote);
}

INetEventer* CTlsContext::onAccept(const CNetAddress& local) {
    return this;
}

s32 CTlsContext::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    mTypeClient = true;
    mHandshake = true;
    mPacket.setUsed(0);
    mID = sessionID;
    mLocal = local;
    bool ret = init(mTypeClient, mHost.c_str(), "CLS");
    handshake(true);
    CLogger::log(ELOG_INFO, "CTlsContext::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID, local.getIPString(), local.getPort(), remote.getIPString(), remote.getPort());
    APP_ASSERT(mEvent);
    return 0;// mEvent->onConnect(sessionID, local, remote);
}


s32 CTlsContext::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    CLogger::log(ELOG_INFO, "CTlsContext::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID, local.getIPString(), local.getPort(), remote.getIPString(), remote.getPort());
    mHandshake = true;
    mPacket.setUsed(0);
    mID = 0;
    mbedtls_ssl_session_reset(&mSSL);
    APP_ASSERT(mEvent);
    return mEvent->onDisconnect(sessionID, local, remote);
}


s32 CTlsContext::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    APP_ASSERT(mEvent);
    return mEvent->onSend(sessionID, buffer, size, result);
}


s32 CTlsContext::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    APP_ASSERT(mEvent);
    return mEvent->onDisconnect(sessionID, local, remote);
}


s32 CTlsContext::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 iSize) {
    mPacket.addBuffer(buffer, iSize);
    s32 ret = -1;
    if(mHandshake) {
        do {
            ret = handshake(false);
        } while(0 != ret && 1 != ret);

        if(0 == ret) {//done
            mHandshake = false;

            u32 flags = mbedtls_ssl_get_verify_result(&mSSL);
            // In real life, we probably want to bail out when ret != 0
            if(flags != 0) {
                s8 vrfy_buf[512];
                mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "", flags);
                CLogger::logInfo("CNetHttpsClient::onReceive", "ssl_verify=%s", vrfy_buf);
            }
            APP_ASSERT(mEvent);
            if(mTypeClient) {
                mEvent->onConnect(sessionID, mLocal, remote);
            } else {
                mEvent->onLink(sessionID, mLocal, remote);
            }
        } else {
            return 0;
        }
    }

    //reading...
    s8 buf[1024];
    do {
        ret = mbedtls_ssl_read(&mSSL, (u8*)buf, sizeof(buf) - 1);
        if(ret > 0) {
            APP_ASSERT(mEvent);
            mEvent->onReceive(remote, sessionID, buf, ret);
        } else if(ret < 0) {
            CLogger::logError("CNetHttpsClient::onReceive", "mbedtls_ssl_read=%d", ret);
            break;
        }
    } while(mPacket.getSize() > 0);
    return 0;
}


//@return 0 = success, <0 = continue, 1 = send, 2 = receive
s32 CTlsContext::handshake(bool currSend) {
    s32 ret;

    while(true) {
        ret = mbedtls_ssl_handshake_step(&mSSL);
        switch(ret) {
        case 0: //success
            if(MBEDTLS_SSL_HANDSHAKE_OVER != mSSL.state) {
                continue;
            }
            return 0;

        case MBEDTLS_ERR_SSL_CONN_EOF://no buffer to read
            return currSend ? 2 : 1;

        default:
            return -3;
        }
    }//while

    return -1;
}

}//namespace net
}//namespace app
