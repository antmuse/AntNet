#include "CNetHttpsClient.h"
#include "CNetService.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "CNetServerAcceptor.h"
#include "CNetEchoServer.h"


namespace irr {
namespace net {

//#define SERVER_NAME "www.baidu.com"
#define SERVER_NAME "127.0.0.1"
#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"


static s32 AppTlsSend(void* ctx, const u8* buf, u64 len) {
    CNetHttpsClient* nd = reinterpret_cast<CNetHttpsClient*>(ctx);
    return nd->sendBuffer(buf, static_cast<s32>(len));
}

static s32 AppTlsRecieve(void* ctx, u8* buf, u64 len) {
    CNetHttpsClient* nd = reinterpret_cast<CNetHttpsClient*>(ctx);
    CNetPacket& pk = nd->getReadCache();
    u32 red = pk.getSize();
    if (red == 0) {
        //printf("getStateOfTLS()=%d\n", nd->getStateOfTLS());
        //Sleep(1000);
        return 0;
    }
    if (red > len) { red = static_cast<u32>(len); }
    memcpy(buf, pk.getConstPointer(), red);
    pk.clear(red);
    return red;
}


static void AppTlsDebug(void *ctx, int level,
    const char *file, int line,
    const char *str) {
    ((void)level);

    printf("%s:%04d: %s", file, line, str);
}


CNetHttpsClient::CNetHttpsClient() :
    mPacket(1024 * 16),
    mHandshake(true),
    mAutoConnect(true),
    mHub(0),
    mSession(0) {

    //mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    //mbedtls_x509_crt_init(&mCertCA);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    const char *pers = "ssl_client1";
    s32 ret = 0;
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const u8*)pers,
        strlen(pers))) != 0) {
        printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        APP_ASSERT(0);
    }

    if (!mCertCA.loadFromFile("D:/App/SSH/client.cer")) {
        printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        APP_ASSERT(0);
    }

    if ((ret = mbedtls_ssl_config_defaults(&conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        APP_ASSERT(0);
    }

    /* OPTIONAL is not optimal for security,
    * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &mCertCA.getCert(), nullptr);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, AppTlsDebug, stdout);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        APP_ASSERT(0);
    }
    if ((ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        APP_ASSERT(0);
    }
}


CNetHttpsClient::~CNetHttpsClient() {
    mbedtls_ssl_close_notify(&ssl);
    //mbedtls_net_free(&server_fd);

    //mbedtls_x509_crt_free(&mCertCA);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}


s32 CNetHttpsClient::onLink(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_ERROR, "CNetHttpsClient::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    mSession = sessionID;
    return ret;
}

//@return 0 = success, <0 = continue, 1 = send, 2 = receive
s32 CNetHttpsClient::handshake(bool currSend) {
    s32 ret;

    while (true) {
        ret = mbedtls_ssl_handshake_step(&ssl);
        switch (ret) {
        case 0: //success
            if (MBEDTLS_SSL_HANDSHAKE_OVER != ssl.state) {
                continue;
            }
            return 0;

        case MBEDTLS_ERR_SSL_WANT_READ:
            return 2;

        case MBEDTLS_ERR_SSL_WANT_WRITE:
            return 1;

            //need stop
        case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
            return -2;

            //call again
        case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
        case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
            continue;

            //no buffer to read
        case MBEDTLS_ERR_SSL_CONN_EOF:
            return currSend ? 2 : 1;

        default:
            return -3;
        }
    }//while

    return -1;
}

s32 CNetHttpsClient::onConnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    mHandshake = true;
    mPacket.setUsed(0);
    mSession = sessionID;

    IAppLogger::log(ELOG_INFO, "CNetHttpsClient::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );

    mbedtls_ssl_set_bio(&ssl, this, AppTlsSend, AppTlsRecieve, NULL);

    s32 ret = handshake(true);
    return ret;
}


s32 CNetHttpsClient::sendBuffer(const void* buf, s32 len) {
    s32 ret = 0;
    if (mSession > 0) {
        ret = mHub->send(mSession, buf, len);
    }
    if (len != ret) {
        IAppLogger::log(ELOG_INFO, "CNetHttpsClient::send", "fail");
    }
    return ret;
}

s32 CNetHttpsClient::onDisconnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    mHandshake = true;
    mbedtls_ssl_session_reset(&ssl);
    s32 ret = 0;
    mPacket.setUsed(0);
    IAppLogger::log(ELOG_INFO, "CNetHttpsClient::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    mSession = 0;
    if (mAutoConnect) {
        mSession = mHub->connect(remote, this);
        if (0 == mSession) {
            APP_ASSERT(0);
            IAppLogger::log(ELOG_ERROR, "CNetHttpsClient::onDisconnect", "[can't got session now-----]");
        }
    }
    return ret;
}


s32 CNetHttpsClient::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CNetHttpsClient::onSend", "[%u],result=%d,size=%d",
        sessionID, result, size);
    return ret;
}


s32 CNetHttpsClient::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    APP_ASSERT(mSession == sessionID);
    IAppLogger::log(ELOG_INFO, "CNetHttpsClient::onReceive", "[%u],size=%d", sessionID, size);
    s32 ret;
    mPacket.addBuffer(buffer, size);
    if (mHandshake) {
        do {
            ret = handshake(false);
        } while (0 != ret && 1 != ret);
        if (0 != ret) {
            return ret;
            APP_ASSERT(0);
        }
        if (0 == ret) {//done
            mHandshake = false;
            u32 flags;
            // In real life, we probably want to bail out when ret != 0
            if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
                char vrfy_buf[512];
                printf(" failed\n");
                mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
                printf("%s\n", vrfy_buf);
            } else {
                printf(" ok\n");
            }
            //IAppLogger::log(ELOG_INFO, "CNetHttpsClient::onReceive", "ssl.size=%llu", ssl.in_left);

            //request
            u64 len = strlen(GET_REQUEST);
            while ((ret = mbedtls_ssl_write(&ssl, (const u8*)GET_REQUEST, len)) <= 0) {
                if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
                    APP_ASSERT(0);
                }
            }
        }
        return ret;
    }


    c8 buf[1024];
    //response
    do {
        ret = mbedtls_ssl_read(&ssl, (u8*)buf, sizeof(buf) - 1);
        if (ret > 0) {
            buf[ret] = 0;
            printf("%s", buf);
        } else if (ret < 0) {
            //printf("mbedtls_ssl_read returned %d\n\n", ret);
            if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                continue;
            }
            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                break;
            }
        }
    } while (mPacket.getSize() > 0);

    return ret;
}

s32 CNetHttpsClient::onTimeout(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CNetHttpsClient::onTimeout", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return 2;
}

}//namespace net
}//namespace irr