#include "CDefaultNetEventer.h"
#include "CNetServiceTCP.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "INetSession.h"
#include "CNetServerAcceptor.h"

namespace irr {
namespace net {

CDefaultNetEventer::CDefaultNetEventer() :
    mPacket(1024 * 8),
    mAutoConnect(false),
    mServer(0),
    mHub(0),
    mSession(0) {
}


CDefaultNetEventer::~CDefaultNetEventer() {
}


s32 CDefaultNetEventer::onLink(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    if(mServer) {
        mServer->setEventer(sessionID, this);
    }
    return ret;
}


s32 CDefaultNetEventer::onConnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    mSession = sessionID;
    mHub->send(mSession, "Hey,server", 11);
    return ret;
}


s32 CDefaultNetEventer::onDisconnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    mPacket.clear(0xFFFFFFFF);
    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    if(mAutoConnect) {
        mSession = mHub->connect(remote, this);
        if(0 == mSession) {
            APP_ASSERT(0);
            IAppLogger::log(ELOG_ERROR, "CDefaultNetEventer::onDisconnect", "[can't got session now-----]");
        }
    }
    return ret;
}


s32 CDefaultNetEventer::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onSend", "[%u,%d,%d,%s]",
        sessionID,
        result,
        size,
        reinterpret_cast<c8*>(buffer)
    );
    //mHub->send(mSession, buffer, size);
    return ret;
}


s32 CDefaultNetEventer::onReceive(u32 sessionID, void* buffer, s32 size) {
    s32 ret = 0;
    mPacket.addBuffer(buffer, size);
    for(ret = 0; mPacket.getReadSize() - ret >= 11; ret += 11) {
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onReceive", "[%u,%d,%s]",
            sessionID, size, mPacket.getReadPointer() + ret);
        if(mServer) {
            mServer->send(sessionID, mPacket.getReadPointer() + ret, 11);
        } else {
            mHub->send(mSession, mPacket.getReadPointer() + ret, 11);
        }
    }
    mPacket.clear(ret);
    return ret;
}


}//namespace net
}//namespace irr