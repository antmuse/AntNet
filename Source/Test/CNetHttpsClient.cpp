#include "CNetHttpsClient.h"
#include "CNetService.h"
#include "CThread.h"
#include "CLogger.h"
#include "CNetPacket.h"
#include "CNetServerAcceptor.h"
#include "CNetEchoServer.h"
#include "HAtomicOperator.h"


namespace app {
namespace net {


////模拟微信请求
//s8* GET_REQUEST =
//"GET /v1/arch/notify/sms/reply/wels?MsgId=200701090749 HTTP/1.1\r\n"
//"Host: callback.danke.com\r\n"
//"Connection: keep-alive\r\n"
//"Upgrade-Insecure-Requests: 1\r\n"
//"User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36 QBCore/4.0.1295.400 QQBrowser/9.0.2524.400 Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2875.116 Safari/537.36 NetType/WIFI MicroMessenger/7.0.5 WindowsWechat\r\n"
//"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
//"Accept-Encoding: gzip, deflate\r\n"
//"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.6,en;q=0.5;q=0.4\r\n"
//"\r\n";


//模拟普通请求
s8* GET_REQUEST = "GET / HTTP/1.0\r\n\r\n";

CNetHttpsClient::CNetHttpsClient() :
    mPacket(1024) {
    mTlsCtx.setEventer(this);
}


CNetHttpsClient::~CNetHttpsClient() {
}


s32 CNetHttpsClient::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    APP_ASSERT(0);
    return 0;
}


s32 CNetHttpsClient::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    mPacket.setUsed(0);
    CLogger::log(ELOG_INFO, "CNetHttpsClient::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID, local.getIPString(), local.getPort(), remote.getIPString(), remote.getPort());

    s32 ret = (s32)strlen(GET_REQUEST);
    ret = send(GET_REQUEST, ret);
    return ret;
}


s32 CNetHttpsClient::send(const void* buf, s32 len) {
    return mTlsCtx.send(buf, len);
}


s32 CNetHttpsClient::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    mPacket.setUsed(0);
    return ret;
}


s32 CNetHttpsClient::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    //CLogger::log(ELOG_INFO, "CNetHttpsClient::onSend", "[%u],result=%d,size=%d", sessionID, result, size);
    return 0;
}


s32 CNetHttpsClient::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    printf("%.*s", size, (s8*)buffer);
    return 0;
}

s32 CNetHttpsClient::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    CLogger::log(ELOG_INFO, "CNetHttpsClient::onTimeout", "[%u,%s:%u->%s:%u]",
        sessionID, local.getIPString(), local.getPort(), remote.getIPString(), remote.getPort());
    return ret;
}

}//namespace net
}//namespace app
