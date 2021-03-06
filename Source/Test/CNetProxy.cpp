#include "CNetProxy.h"
#include "CNetService.h"
#include "CLogger.h"
#include "CNetServerAcceptor.h"
#include <conio.h>

namespace app {
namespace net {

void CNetProxyNode::grab() {
    AppAtomicIncrementFetch(&mUseCount);
}

bool CNetProxyNode::drop() {
    s32 cur = AppAtomicDecrementFetch(&mUseCount);
    if (0 == cur) {
        delete this;
    }
    return 0 == cur;
}

s32 CNetProxyNode::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    APP_ASSERT(mFrontNet);
    mConnetID = sessionID;
    mStatus = 2;
    s32 smax = 1000;
    while (smax > 0 && 2 != mPairNode->mStatus) {
        CThread::sleep(10);
        smax -= 10;
    }
    if (2 != mPairNode->mStatus) {
        setPairNode(nullptr);
        mProxyHub->getServer().disconnect(sessionID);
    }
    return mProxyHub->onLink(sessionID, local, remote);
}

s32 CNetProxyNode::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    mConnetID = sessionID;
    mStatus = 2;
    return mProxyHub->onConnect(sessionID, local, remote);
}

s32 CNetProxyNode::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {

    return 0;
}

s32 CNetProxyNode::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    mConnetID = 0;
    if (mPairNode) {
        if (mPairNode->mUseCount > 1) {
            mProxyHub->getServer().disconnect(mPairNode->mConnetID);
        }
        setPairNode(nullptr);
    }
    if (2 == mStatus) {//mStatus<2意味着之前没有连接成功，未调用onConnect()
        mProxyHub->onDisconnect(sessionID, local, remote);
    }
    mStatus = 0;
    drop();
    return 0;
}

s32 CNetProxyNode::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    if (mFrontNet) {
        return mProxyHub->onSend(sessionID, buffer, size, result);
    }
    return 0;
}

s32 CNetProxyNode::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    s32 ret = 0;
    if (mPairNode) {
        ret = mProxyHub->getServer().send(mPairNode->mConnetID, buffer, size);
    }
    return ret;
}



//--------------------------------------------------------------------------

CNetProxy::CNetProxy() :
    mSendSuccessBytes(0),
    mSendFailBytes(0),
    mDislinkCount(0),
    mLinkCount(0) {
}

CNetProxy::~CNetProxy() {
}

void CNetProxy::show() {
    s32 key = '\0';
    printf("CNetProxy::show>>Press [ESC] to quit");
    while (!_kbhit() || (key = _getch()) != 27) {
        printf("CNetProxy, TCP=%u/%u, sent=%uKb+%uKb\n",
            mDislinkCount, mLinkCount, mSendSuccessBytes >> 10, mSendFailBytes >> 10);
        CThread::sleep(1000);
    }
    printf("CNetProxy::show>>exit...");
}

bool CNetProxy::stop() {
    mLisenServer.stop();
    mServer.stop();

    while (mDislinkCount != mLinkCount) {
        CThread::sleep(100);
    }
    return true;
}

bool CNetProxy::start(u16 port) {
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

void CNetProxy::setProxyHost(const s8* host) {
    mRemoteAddr.setIPort(host);
}


s32 CNetProxy::onTimeout(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    APP_ASSERT(0);
    return 0;
}


INetEventer* CNetProxy::onAccept(const CNetAddress& local) {
    CNetProxyNode* front = new CNetProxyNode(this, true);
    CNetProxyNode* back = new CNetProxyNode(this, false);
    front->setPairNode(back);
    back->setPairNode(front);
    u32 cid = mServer.connect(mRemoteAddr, back);
    if (cid > 0) {
        back->mConnetID = cid;
        back->mStatus = 1;
    } else {
        delete front;
        delete back;
        front = nullptr;
    }
    return front;
}


s32 CNetProxy::onLink(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    CLogger::log(ELOG_INFO, "CNetProxy::onLink", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    AppAtomicIncrementFetch(&mLinkCount);
    return 0;
}


s32 CNetProxy::onConnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    CLogger::log(ELOG_INFO, "CNetProxy::onConnect", "[%u,%s:%u->%s:%u]",
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    AppAtomicIncrementFetch(&mLinkCount);
    return 0;
}


s32 CNetProxy::onDisconnect(u32 sessionID, const CNetAddress& local, const CNetAddress& remote) {
    s32 ret = AppAtomicIncrementFetch(&mDislinkCount);
    CLogger::log(ELOG_INFO, "CNetProxy::onDisconnect", "%d[%u,%s:%u->%s:%u]",
        ret,
        sessionID,
        local.getIPString(),
        local.getPort(),
        remote.getIPString(),
        remote.getPort()
    );
    return ret;
}


s32 CNetProxy::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
    s32 ret = 0;
    AppAtomicFetchAdd(size, 0 == result ? &mSendSuccessBytes : &mSendFailBytes);
    return ret;
}


s32 CNetProxy::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
    APP_ASSERT(0);
    return 0;
}


}//namespace net
}//namespace app