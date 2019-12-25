#include "CNetService.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "CMemoryHub.h"
#include "CNetUtility.h"


#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace irr {
namespace net {

void CNetServiceTCP::run() {
    const u64 tmgap = mWheel.getInterval();
    u64 lastshow = mCurrentTime;
    CNetSocket sock(mPoller.getSocketPair().getSocketA());
    const u32 maxe = mConfig->mMaxFetchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    CNetSession* iContext;
    u64 last = mCurrentTime;
    u32 gotsz = 0;
    mWheel.setCurrent(mCurrentTime);
    u32 action;
    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, mTimeInterval);
        if(gotsz > 0) {
            mCurrentTime = IAppTimer::getTime();
            for(u32 i = 0; i < gotsz; ++i) {
                s32 ret = 1;
                if(iEvent[i].mData.mData32 < ENET_SESSION_MASK) {
                    iContext = mSessionPool.getSession(iEvent[i].mData.mData32);
                    //mTotalReceived += iEvent[i].mBytes;
                    if((EPOLLIN | EPOLLERR) & iEvent[i].mEvent) {
                        ret = iContext->postReceive();
                    } else if((EPOLLOUT) & iEvent[i].mEvent) {
                        ret = iContext->postSend(nullptr);
                    } else {

                    }
                } else {
                    u32 mask;
                    s32 ret = sock.receiveAll(&mask, sizeof(mask));
                    if(ret != sizeof(mask)) {
                        //TODO>>
                        continue;
                    }
                    if(ENET_SESSION_MASK == mask) {
                        mRunning = false;
                        break;
                    }
                    action = ENET_CMD_MASK & mask;
                    iContext = mSessionPool.getSession(ENET_SESSION_MASK & mask);

                    switch(action) {
                    case ENET_CMD_SEND:
                        ret = -1;
                        break;
                    case ENET_CMD_RECEIVE:
                        //just add in time wheel
                        break;
                    case ENET_CMD_CONNECT:
                        ret = iContext->postConnect();
                        break;
                    case ENET_CMD_DISCONNECT:
                        ret = iContext->postDisconnect();
                        break;
                    case ENET_CMD_TIMEOUT:
                        ret = iContext->stepTimeout();
                        break;
                    }//switch
                }//if

                if(ret > 0) {
                    mWheel.add(iContext->getTimeNode(),
                        ret > 1 ? mConfig->mSessionTimeout : 1000, 1);
                } else if(0 == ret) {
                    iContext->stepClose();             //step 1
                    remove(iContext);                  //step 2
                }
            }//for
            if(mCurrentTime - last >= tmgap) {
                last = mCurrentTime;
                mWheel.update(mCurrentTime);
            }
        } else {//poller error
            const s32 pcode = mPoller.getError();
            switch(pcode) {
            case EINTR://epoll±»ÐÅºÅÖÐ¶Ï
                break;
            default:
                break;
            }
            mCurrentTime = IAppTimer::getTime();
            mWheel.update(mCurrentTime);
            last = mCurrentTime;
            if(mCurrentTime - lastshow >= 1000) {
                IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run",
                    "[context:%u/%u][socket:%u/%u]",
                    mSessionPool.getActiveCount(), mSessionPool.getMaxContext(),
                    mClosedSocket, mCreatedSocket);
                lastshow = mCurrentTime;
            }
            //clearError(iContext, iAction);
            continue;
        }//if

        dispatchBuffers();
    }//for

    delete[] iEvent;
    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run", "thread exited");
}


bool CNetServiceTCP::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::clearError",
            "operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch

    return false;
}

}//namespace net
}//namespace irr
#endif //APP_PLATFORM_LINUX
