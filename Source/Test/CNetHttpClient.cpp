#include "CNetHttpClient.h"
#include "CNetService.h"
#include "CLogger.h"

namespace app {
namespace net {

int on_message_begin(http_parser* iContext) {
    (void)iContext;
    printf("\n***HTTP BEGIN***\n\n");
    return 0;
}

int on_headers_complete(http_parser* iContext) {
    (void)iContext;
    printf("\n***HEADERS COMPLETE, Status=%u***\n\n", iContext->status_code);
    return 0;
}

int on_message_complete(http_parser* iContext) {
    (void)iContext;
    printf("\n***MESSAGE COMPLETE***\n\n");
    return 0;
}

int on_url(http_parser* iContext, const char* at, size_t length) {
    (void)iContext;
    printf("Url: %.*s\n", (int)length, at);
    return 0;
}

int on_header_field(http_parser* iContext, const char* at, size_t length) {
    (void)iContext;
    printf("Header field: %.*s : ", (int)length, at);
    return 0;
}

int on_header_value(http_parser* iContext, const char* at, size_t length) {
    (void)iContext;
    printf("%.*s\n", (int)length, at);
    return 0;
}

int on_body(http_parser* iContext, const char* at, size_t length) {
    (void)iContext;
    printf("Body: %.*s\n", (int)length, at);
    return 0;
}


CNetHttpClient::CNetHttpClient() :
    mPack(1024),
    mServer(nullptr) {

    http_parser_settings_init(&mSets);
    mSets.on_message_begin = on_message_begin;
    mSets.on_url = on_url;
    mSets.on_header_field = on_header_field;
    mSets.on_header_value = on_header_value;
    mSets.on_headers_complete = on_headers_complete;
    mSets.on_body = on_body;
    mSets.on_message_complete = on_message_complete;

    mParserType = HTTP_RESPONSE;

    http_parser_init(&mParser, mParserType);
    mParser.data = this;
}


CNetHttpClient::~CNetHttpClient() {
}



//GET /hash/hkeys.html HTTP/1.1
//Host: redisdoc.com
//Connection: keep-alive
//Cache-Control: max-age=0
//Upgrade-Insecure-Requests: 1
//User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.132 Safari/537.36
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
//Accept-Encoding: gzip, deflate
//Accept-Language: zh-CN,zh;q=0.9,en;q=0.8
//Cookie: _ga=GA1.2.1402562334.1587380379; _gid=GA1.2.190891489.1588990983
//If-None-Match: W/"5da7c8c6-2ebb"
//If-Modified-Since: Thu, 17 Oct 2019 01:49:58 GMT
bool CNetHttpClient::request(const s8* url) {
    if (!mServer || !mURL.set(url)) {
        return false;
    }
    CNetAddress ad(mURL.getPort());
    core::CString host;
    mURL.getPath(host);
    mPack.setUsed(0);
    mPack.addBuffer("GET ", (u32)sizeof("GET ") - 1);
    mPack.addBuffer(host.c_str(), host.size());
    mURL.getHost(host);
    ad.setDomain(host.c_str());
    mPack.addBuffer(" HTTP/1.1\r\n", (u32)sizeof(" HTTP/1.1\r\n") - 1);

    mPack.addBuffer("Host:", (u32)sizeof("Host:") - 1);
    mPack.addBuffer(host.c_str(), host.size());
    mPack.addBuffer("\r\n", (u32)sizeof("\r\n") - 1);

    mPack.addBuffer("User-Agent:", (u32)sizeof("User-Agent:") - 1);
    mPack.addBuffer("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.132 Safari/537.36\r\n",
        (u32)sizeof("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.132 Safari/537.36\r\n") - 1);

    mPack.addBuffer("Accept:", (u32)sizeof("Accept:") - 1);
    mPack.addBuffer("text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n",
        (u32)sizeof("text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n") - 1);

    mPack.addBuffer("Accept-Encoding:", (u32)sizeof("Accept-Encoding:") - 1);
    mPack.addBuffer("deflate\r\n", (u32)sizeof("deflate\r\n") - 1);

    mPack.addBuffer("Accept-Language:", (u32)sizeof("Accept-Language:") - 1);
    mPack.addBuffer("zh-CN,zh;q=0.9,en;q=0.8\r\n", (u32)sizeof("zh-CN,zh;q=0.9,en;q=0.8\r\n") - 1);

    mPack.addBuffer("\r\n", (u32)sizeof("\r\n") - 1);
    //mPack.addBuffer("\r\n\r\n", (u32)sizeof("\r\n\r\n") - 1);

    if (mPack.getSize() < mPack.getAllocatedSize()) {
        mPack[mPack.getSize()] = 0;
    }
    u32 id = mServer->connect(ad, this);
    return id > 0;
}

s32 CNetHttpClient::onTimeout(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_INFO, "CNetHttpClient::onTimeout", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return 0;
}

s32 CNetHttpClient::onLink(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_INFO, "CNetHttpClient::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return ret;
}


s32 CNetHttpClient::onConnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_ERROR, "CNetHttpClient::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );

    mServer->send(sessionID, mPack.getConstPointer(), mPack.getSize());
    mPack.setUsed(0);
    return ret;
}


s32 CNetHttpClient::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    CLogger::log(ELOG_ERROR, "CNetHttpClient::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return 0;
}


s32 CNetHttpClient::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    s32 ret = 0;
    CLogger::log(ELOG_ERROR, "CNetHttpClient::onSend", "[%u,%d,%d]",
        sessionID,
        result,
        size
    );
    return ret;
}


s32 CNetHttpClient::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    APP_ASSERT(size > 0);

    if (HPE_OK != mParser.http_errno) {
        CLogger::log(ELOG_ERROR, "CNetHttpClient::onReceive", "[parser ecode=%u]", mParser.http_errno);
        return 0;
    }
    if (mPack.getSize() > 0) {
        mPack.addBuffer(buffer, size);
        u64 parsed = http_parser_execute(&mParser, &mSets, mPack.getConstPointer(), mPack.getSize());
        mPack.clear((u32)parsed);
    } else {
        s32 parsed = (s32)http_parser_execute(&mParser, &mSets, static_cast<const s8*>(buffer), size);
        if (parsed < size) {
            mPack.addBuffer(static_cast<const s8*>(buffer) + parsed, size - parsed);
        }
    }
    return 0;
}


}//namespace net
}//namespace app
