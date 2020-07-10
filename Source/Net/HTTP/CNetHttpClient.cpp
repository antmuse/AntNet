#include "CNetHttpClient.h"
#include "CNetService.h"
#include "CLogger.h"

namespace app {
namespace net {

s32 AppOnHttpBegin(http_parser* iContext) {
    (void)iContext;
    //printf("\n***HTTP BEGIN***\n\n");
    return 0;
}

s32 AppOnHttpStatus(http_parser* iContext, const s8* at, usz length) {
    //printf("AppOnHttpStatus>>status=%u, str=%.*s\n", iContext->status_code, (s32)length, at);
    CNetHttpClient& nd = *(CNetHttpClient*)iContext->data;
    CNetHttpResponse& resp = nd.getResp();
    resp.setStatus(iContext->status_code);
    return 0;
}

s32 AppOnHeaderFinish(http_parser* iContext) {
    (void)iContext;
    return 0;
}

s32 AppOnHttpFinish(http_parser* iContext) {
    CNetHttpClient& nd = *(CNetHttpClient*)iContext->data;
    CNetHttpResponse& resp = nd.getResp();
    INetEventerHttp* evt = nd.getEventer();
    resp.setFinished(true);
    if(evt) {
        evt->onResponse(nd.getRequest(), resp);
    } else {
        resp.show(true);
    }
    return 0;
}

s32 AppOnURL(http_parser* iContext, const s8* at, usz length) {
    (void)iContext;
    printf("Url=%.*s\n", (s32)length, at);
    return 0;
}

s32 AppOnHeaderField(http_parser* iContext, const s8* at, usz length) {
    //printf("%.*s : ", (s32)length, at);
    CNetHttpClient& nd = *(CNetHttpClient*)iContext->data;
    CNetHttpResponse& resp = nd.getResp();
    resp.setCurrentHeader(at, (u32)length);
    return 0;
}

s32 AppOnHeaderValue(http_parser* iContext, const s8* at, usz length) {
    //printf("%.*s\n", (s32)length, at);
    CNetHttpClient& nd = *(CNetHttpClient*)iContext->data;
    CNetHttpResponse& resp = nd.getResp();
    resp.setCurrentValue(at, (u32)length);
    return 0;
}

s32 AppOnHttpBody(http_parser* iContext, const s8* at, usz length) {
    //printf("%.*s\n", (s32)length, at);
    CNetHttpClient& nd = *(CNetHttpClient*)iContext->data;
    CNetHttpResponse& resp = nd.getResp();
    resp.appendBuf(at, (u32)length);
    return 0;
}

//Transfer-Encoding : chunked
s32 AppOnChunkHeader(http_parser* iContext) {
    (void)iContext;
    //printf("AppOnChunkHeader>>chunk size=%llu\n", iContext->content_length);
    return 0;
}

s32 AppOnHttpChunk(http_parser* iContext) {
    CNetHttpClient& nd = *(CNetHttpClient*)iContext->data;
    CNetHttpResponse& resp = nd.getResp();
    INetEventerHttp* evt = nd.getEventer();
    if(evt) {
        evt->onResponse(nd.getRequest(), resp);
    } else {
        resp.show(false);
    }
    resp.getPage().setUsed(0);
    return 0;
}

CNetHttpClient::CNetHttpClient() :
    mEvent(nullptr),
    mPack(512),
    mServer(nullptr) {

    http_parser_settings_init(&mSets);
    mSets.on_message_begin = AppOnHttpBegin;
    mSets.on_url = AppOnURL;
    mSets.on_status = AppOnHttpStatus;
    mSets.on_header_field = AppOnHeaderField;
    mSets.on_header_value = AppOnHeaderValue;
    mSets.on_headers_complete = AppOnHeaderFinish;
    mSets.on_body = AppOnHttpBody;
    mSets.on_message_complete = AppOnHttpFinish;
    mSets.on_chunk_header = AppOnChunkHeader;
    mSets.on_chunk_complete = AppOnHttpChunk;

    mParserType = HTTP_RESPONSE;

    http_parser_init(&mParser, mParserType);
    mParser.data = this;
}


CNetHttpClient::~CNetHttpClient() {
}

//Î¢ÐÅPC°æ User-Agent
//"User-Agent : Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36 QBCore/4.0.1295.400 QQBrowser/9.0.2524.400 Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2875.116 Safari/537.36 NetType/WIFI MicroMessenger/7.0.5 WindowsWechat"
//PC Chrome User-Agent
//"User-Agent : Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.132 Safari/537.36"
bool CNetHttpClient::httpGet(const s8* url) {
    if(!mServer || !mRequest.getURL().set(url)) {
        return false;
    }
    mRequest.clear();
    mRequest.setMethod("GET");
    mRequest.setVersion("1.1");
    mRequest.setKeepAlive(false);
    mRequest.getHead().setValue("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.132 Safari/537.36");
    mRequest.getHead().setValue("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
    mRequest.getHead().setValue("Accept-Encoding", "zh-CN,zh;q=0.9,en;q=0.8");
    mRequest.getBuffer(mPack);

    CNetAddress ad(mRequest.getURL().getPort());
    core::CString host;
    mRequest.getURL().getHost(host);
    ad.setDomain(host.c_str());
    u32 id = mServer->connect(ad, this);
    return id > 0;
}

s32 CNetHttpClient::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_INFO, "CNetHttpClient::onTimeout", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(), local.getPort(),
        remote.getIPString(), remote.getPort()
    );
    return 0;
}

s32 CNetHttpClient::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_INFO, "CNetHttpClient::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(), local.getPort(),
        remote.getIPString(), remote.getPort()
    );
    return ret;
}


s32 CNetHttpClient::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_INFO, "CNetHttpClient::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(), local.getPort(),
        remote.getIPString(), remote.getPort()
    );

    mResp.clear();
    mServer->send(sessionID, mPack.getConstPointer(), mPack.getSize());
    mPack.setUsed(0);
    return ret;
}


s32 CNetHttpClient::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    CLogger::log(ELOG_INFO, "CNetHttpClient::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(), local.getPort(),
        remote.getIPString(), remote.getPort()
    );
    //mPack.shrink(1024);
    return 0;
}


s32 CNetHttpClient::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    return 0;
}


s32 CNetHttpClient::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    APP_ASSERT(size > 0);

    if(HPE_OK != mParser.http_errno) {
        CLogger::log(ELOG_ERROR, "CNetHttpClient::onReceive", "[parser ecode=%u]", mParser.http_errno);
        return 0;
    }
    if(mPack.getSize() > 0) {
        mPack.addBuffer(buffer, size);
        u64 parsed = http_parser_execute(&mParser, &mSets, mPack.getConstPointer(), mPack.getSize());
        mPack.clear((u32)parsed);
    } else {
        s32 parsed = (s32)http_parser_execute(&mParser, &mSets, static_cast<const s8*>(buffer), size);
        if(parsed < size) {
            mPack.addBuffer(static_cast<const s8*>(buffer) + parsed, size - parsed);
        }
    }
    return 0;
}


}//namespace net
}//namespace app
