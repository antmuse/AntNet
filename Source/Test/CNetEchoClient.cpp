#include "CNetEchoClient.h"
#include "CNetServiceTCP.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "CNetServerAcceptor.h"
#include "CNetEchoServer.h"

namespace irr {
namespace net {

s32 CNetEchoClient::mMaxSendPackets = 20000;
s32 CNetEchoClient::mSendRequest = 0;
s32 CNetEchoClient::mSendRequestFail = 0;
s32 CNetEchoClient::mSendSuccess = 0;
s32 CNetEchoClient::mSendFailBytes = 0;
s32 CNetEchoClient::mSendFail = 0;
s32 CNetEchoClient::mRecvCount = 0;
s32 CNetEchoClient::mRecvBadCount = 0;
s32 CNetEchoClient::mSendSuccessBytes = 0;
s32 CNetEchoClient::mRecvBytes = 0;
c8 CNetEchoClient::mTestData[4 * 1024];

void CNetEchoClient::initData(const c8* msg, s32 mb) {
    AppAtomicFetchSet((sizeof(mTestData) - 1 + 1024 * 1024 * mb) / sizeof(mTestData),
        &mMaxSendPackets);
    AppAtomicFetchSet(0, &mSendRequest);
    AppAtomicFetchSet(0, &mSendRequestFail);
    AppAtomicFetchSet(0, &mSendSuccess);
    AppAtomicFetchSet(0, &mSendFail);
    AppAtomicFetchSet(0, &mSendSuccessBytes);
    AppAtomicFetchSet(0, &mSendFailBytes);
    AppAtomicFetchSet(0, &mRecvCount);
    AppAtomicFetchSet(0, &mRecvBadCount);
    AppAtomicFetchSet(0, &mRecvBytes);

    u64 size = 1 + (msg ? strlen(msg) : 0);
    size = core::min_<u64>(size, sizeof(mTestData) - sizeof(u32));
    ::memcpy(mTestData + sizeof(u32), msg ? msg : "", size);
    ::memset(mTestData + sizeof(u32) + size, 'v',
        sizeof(mTestData) - sizeof(u32) - size);
    *(u32*) mTestData = sizeof(mTestData);
}


CNetEchoClient::CNetEchoClient() :
    mPacket(1024 * 16),
    mAutoConnect(false),
    mHub(0),
    mSession(0) {
}


CNetEchoClient::~CNetEchoClient() {
}


s32 CNetEchoClient::onLink(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_ERROR, "CNetEchoClient::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    mSession = sessionID;
    return ret;
}


s32 CNetEchoClient::onConnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    IAppLogger::log(ELOG_INFO, "CNetEchoClient::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    mPacket.setUsed(0);
    mSession = sessionID;
    send();
    return ret;
}

void CNetEchoClient::send() {
    s32 cnt = AppAtomicIncrementFetch(&mSendRequest);
    if(cnt <= mMaxSendPackets) {
        s32 ret = mHub->send(mSession, mTestData, sizeof(mTestData));
        if(sizeof(mTestData) != ret) {
            AppAtomicIncrementFetch(&mSendRequestFail);
        }
    } else {
        AppAtomicDecrementFetch(&mSendRequest);
        IAppLogger::log(ELOG_INFO, "CNetEchoClient::send", "task finished");
    }
}

s32 CNetEchoClient::onDisconnect(u32 sessionID,
    const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = 0;
    mPacket.setUsed(0);
    IAppLogger::log(ELOG_INFO, "CNetEchoClient::onDisconnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    mSession = 0;
    if(mAutoConnect) {
        mSession = mHub->connect(remote, this);
        if(0 == mSession) {
            APP_ASSERT(0);
            IAppLogger::log(ELOG_ERROR, "CNetEchoClient::onDisconnect", "[can't got session now-----]");
        }
    }
    return ret;
}


s32 CNetEchoClient::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    s32 ret = 0;
    if(0 == result) {
        AppAtomicFetchAdd(size, &mSendSuccessBytes);
        AppAtomicIncrementFetch(&mSendSuccess);
        send();
    } else {
        AppAtomicFetchAdd(size, &mSendFailBytes);
        AppAtomicIncrementFetch(&mSendFail);
        IAppLogger::log(ELOG_ERROR, "CNetEchoClient::onSend", "Failed[%u,%d,%d,%s]",
            sessionID,
            result,
            size,
            reinterpret_cast<c8*>(buffer)
        );
    }
    return ret;
}


s32 CNetEchoClient::onReceive(u32 sessionID, void* buffer, s32 size) {
    APP_ASSERT(mSession == sessionID);
    AppAtomicFetchAdd(size, &mRecvBytes);

    s32 ret = 0;
    mPacket.addBuffer(buffer, size);
    for(; mPacket.getReadSize() >= sizeof(mTestData);) {
        ret += sizeof(mTestData);
        if(sizeof(mTestData) == (*(u32*) mPacket.getReadPointer())) {
            /*IAppLogger::log(ELOG_INFO, "CNetEchoClient::onReceive", "[%u,%d,%s]",
            sessionID, size, mPacket.getReadPointer() + ret);*/
            AppAtomicIncrementFetch(&mRecvCount);
        } else {
            AppAtomicIncrementFetch(&mRecvBadCount);
        }
        mPacket.seek(sizeof(mTestData), false);
    }
    mPacket.clear(ret);
    return ret;
}


}//namespace net
}//namespace irr