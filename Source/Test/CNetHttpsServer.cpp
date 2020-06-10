#include "CNetHttpsServer.h"
#include "CNetService.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "CNetServerAcceptor.h"
#include "CNetEchoServer.h"
#include <conio.h>

#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection using</p>\r\n"


namespace irr {
namespace net {

static s32 AppTlsSend(void* ctx, const u8* buf, u64 len) {
    CNetHttpsNode* nd = reinterpret_cast<CNetHttpsNode*>(ctx);
    return nd->sendBuffer(buf, static_cast<s32>(len));
}

static s32 AppTlsRecieve(void* ctx, u8* buf, u64 len) {
    CNetHttpsNode* nd = reinterpret_cast<CNetHttpsNode*>(ctx);
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

static void AppTlsDebug(void *ctx, int level, const char *file, int line, const char *str) {
    ((void)level);

    printf("%s:%04d: %s", file, line, str);
}




CNetHttpsNode::CNetHttpsNode(CNetHttpsServer* hub) :
    mHandshake(true), mConnetID(0), mStatus(0), mHttpsHub(hub), mUseCount(1) {
    mbedtls_ssl_init(&mSSL);
}

CNetHttpsNode::~CNetHttpsNode() {
    mbedtls_ssl_close_notify(&mSSL);
    mbedtls_ssl_free(&mSSL);
}

void CNetHttpsNode::grab() {
    AppAtomicIncrementFetch(&mUseCount);
}

bool CNetHttpsNode::drop() {
    s32 cur = AppAtomicDecrementFetch(&mUseCount);
    if (0 == cur) {
        delete this;
    }
    return 0 == cur;
}

s32 CNetHttpsNode::sendBuffer(const void* buf, s32 len) {
    s32 ret = 0;
    if (mConnetID > 0) {
        ret = mHttpsHub->getServer()->send(mConnetID, buf, len);
    }
    if (len != ret) {
        IAppLogger::log(ELOG_INFO, "CNetHttpsNode::send", "fail");
    }
    return ret;
}

//@return 0 = success, <0 = continue, 1 = send, 2 = receive
s32 CNetHttpsNode::handshake(bool currSend) {
    s32 ret;

    while (true) {
        ret = mbedtls_ssl_handshake_step(&mSSL);
        switch (ret) {
        case 0: //success
            if (MBEDTLS_SSL_HANDSHAKE_OVER != mSSL.state) {
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

s32 CNetHttpsNode::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    mConnetID = sessionID;
    mStatus = 2;
    mHandshake = true;
    mPacket.setUsed(0);
    s32 ret = 0;
    if ((ret = mbedtls_ssl_setup(&mSSL, &mHttpsHub->getTlsConfig())) != 0) {
        printf("CNetHttpsNode::onLink>>mbedtls_ssl_setup returned %d\n", ret);
        APP_ASSERT(0);
    }
    mbedtls_ssl_session_reset(&mSSL);
    mbedtls_ssl_set_bio(&mSSL, this, AppTlsSend, AppTlsRecieve, NULL);
    //handshake(true);
    return mHttpsHub->onLink(sessionID, local, remote);
}

s32 CNetHttpsNode::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    APP_ASSERT(0);
    return 0;
}

s32 CNetHttpsNode::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {

    return 0;
}

s32 CNetHttpsNode::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = mHttpsHub->onDisconnect(sessionID, local, remote);
    mStatus = 0;
    mConnetID = 0;
    mHandshake = true;
    mPacket.setUsed(0);
    mbedtls_ssl_session_reset(&mSSL);
    drop();
    return ret;
}

s32 CNetHttpsNode::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    return mHttpsHub->onSend(sessionID, buffer, size, result);
}

s32 CNetHttpsNode::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
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
            if ((flags = mbedtls_ssl_get_verify_result(&mSSL)) != 0) {
                char vrfy_buf[512];
                printf(" failed\n");
                mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
                printf("%s\n", vrfy_buf);
            } else {
                printf(" ok\n");
            }
            IAppLogger::log(ELOG_INFO, "CNetHttpsNode::onReceive", "[%u],size=%d", sessionID, size);

            //resp
            u64 len = strlen(HTTP_RESPONSE);
            while ((ret = mbedtls_ssl_write(&mSSL, (const u8*)HTTP_RESPONSE, len)) <= 0) {
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
        ret = mbedtls_ssl_read(&mSSL, (u8*)buf, sizeof(buf) - 1);
        if (ret > 0) {
            buf[ret] = 0;
            printf("%.*s", ret, buf);
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


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

CNetHttpsServer::CNetHttpsServer() :
    mLinkCount(0),
    mSendSuccessBytes(0),
    mSendFailBytes(0),
    mDislinkCount(0){

    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_pk_init(&pkey);

    const c8 *pers = "ssl_server";
    s32 ret = 0;
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const u8*)pers,
        strlen(pers))) != 0) {
        printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        APP_ASSERT(0);
    }

    if (!mCertCA.loadFromBuf(mbedtls_test_srv_crt, mbedtls_test_srv_crt_len)) {
        printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        APP_ASSERT(0);
    }
    if (!mCertCA.loadFromBuf(mbedtls_test_cas_pem, mbedtls_test_cas_pem_len)) {
        printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        APP_ASSERT(0);
    }
    ret = mbedtls_pk_parse_key(&pkey, (const u8*)mbedtls_test_srv_key, mbedtls_test_srv_key_len, NULL, 0);
    if (ret != 0) {
        printf(" failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret);
        APP_ASSERT(0);
    }

    if ((ret = mbedtls_ssl_config_defaults(&conf,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        APP_ASSERT(0);
    }

    /* OPTIONAL is not optimal for security,
    * but makes interop easier in this simplified example */
    //mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_ca_chain(&conf, mCertCA.getCert().next, nullptr);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, AppTlsDebug, stdout);
    if ((ret = mbedtls_ssl_conf_own_cert(&conf, &mCertCA.getCert(), &pkey)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        APP_ASSERT(0);
    }
}


CNetHttpsServer::~CNetHttpsServer() {
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}


void CNetHttpsServer::show() {
    s32 key = '\0';
    printf("CNetHttpsServer::show>>Press [ESC] to quit");
    while (!_kbhit() || (key = _getch()) != 27) {
        printf("CNetHttpsServer, TCP=%u/%u, sent=%uKb+%uKb\n",
            mDislinkCount, mLinkCount, mSendSuccessBytes >> 10, mSendFailBytes >> 10);
        CThread::sleep(1000);
    }
    printf("CNetHttpsServer::show>>exit...");
}

bool CNetHttpsServer::stop() {
    mLisenServer.stop();
    mServer.stop();

    while (mDislinkCount < mLinkCount) {
        CThread::sleep(100);
    }
    return true;
}

bool CNetHttpsServer::start(u16 port) {
    mConfig.mReuse = true;
    mConfig.mMaxPostAccept = 8;
    mConfig.mMaxFetchEvents = 28;
    mConfig.mMaxContext = 200;
    mConfig.mPollTimeout = 5;
    mConfig.mSessionTimeout = 30000;
    mConfig.mMaxWorkThread = 5;
    mConfig.check();
    mConfig.print();

    net::CNetAddress addr(port);
    mLisenServer.setLocalAddress(addr);
    mLisenServer.setEventer(this);
    mLisenServer.setServer(&mServer);
    mServer.start(&mConfig);
    mLisenServer.start(&mConfig);
    return true;
}


INetEventer* CNetHttpsServer::onAccept(const CNetAddress& local) {
    return new CNetHttpsNode(this);
    /*static bool once = true;
    CNetHttpsNode* nd = nullptr;
    if (once) {
        nd = new CNetHttpsNode(this);
        once = false;
    }
    return nd;*/
}


s32 CNetHttpsServer::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    AppAtomicIncrementFetch(&mLinkCount);
    IAppLogger::log(ELOG_INFO, "CNetHttpsServer::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return 0;
}


s32 CNetHttpsServer::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    //AppAtomicIncrementFetch(&mLinkCount);
    APP_ASSERT(0);
    return 0;
}


s32 CNetHttpsServer::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = AppAtomicIncrementFetch(&mDislinkCount);
    IAppLogger::log(ELOG_INFO, "CNetHttpsServer::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return 0;
}


s32 CNetHttpsServer::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    return 0;
}


s32 CNetHttpsServer::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    return 0;
}

s32 CNetHttpsServer::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    return 0;
}

}//namespace net
}//namespace irr