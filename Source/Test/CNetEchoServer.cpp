#include "CNetEchoServer.h"
#include "CNetService.h"
#include "IAppLogger.h"
#include "CNetServerAcceptor.h"

namespace irr {
namespace net {

CNetEchoServer::CNetEchoServer() :
    mServer(0),
    mSendSuccessBytes(0),
    mSendFailBytes(0),
    mRecvBytes(0),
    mSendBytes(0),
    mDislinkCount(0),
    mLinkCount(0) {
}


CNetEchoServer::~CNetEchoServer() {
}

void CNetEchoServer::setServer(CNetServerAcceptor* hub) {
    if(hub) {
        hub->setEventer(this);
        mServer = hub;
    }
}

s32 CNetEchoServer::onTimeout(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CNetEchoServer::onTimeout", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return 0;
}

s32 CNetEchoServer::onLink(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CNetEchoServer::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    AppAtomicIncrementFetch(&mLinkCount);
    //mServer->setEventer(sessionID, this);
    return ret;
}


s32 CNetEchoServer::onConnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_ERROR, "CNetEchoServer::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return ret;
}


s32 CNetEchoServer::onDisconnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = AppAtomicIncrementFetch(&mDislinkCount);
    IAppLogger::log(ELOG_ERROR, "CNetEchoServer::onDisconnect", "%d[%u,%s:%u->%s:%u]",
        ret,
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return ret;
}


s32 CNetEchoServer::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    s32 ret = 0;
    /*IAppLogger::log(ELOG_ERROR, "CNetEchoServer::onSend", "[%u,%d,%d]",
        sessionID,
        result,
        size
    );*/
    AppAtomicFetchAdd(size, 0 == result ? &mSendSuccessBytes : &mSendFailBytes);
    return ret;
}


s32 CNetEchoServer::onReceive(u32 sessionID, void* buffer, s32 size) {
    /*IAppLogger::log(ELOG_ERROR, "CNetEchoServer::onReceive", "[%u,%d]",
        sessionID, size);*/
    APP_ASSERT(size > 0);

    AppAtomicFetchAdd(size, &mRecvBytes);

    s32 ret = mServer->send(sessionID, buffer, size);
    if(ret == size) {
        AppAtomicFetchAdd(size, &mSendBytes);
    }
    return ret;
}


}//namespace net
}//namespace irr