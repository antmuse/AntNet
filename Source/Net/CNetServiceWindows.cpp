#include "CNetService.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "CMemoryHub.h"
#include "CNetUtility.h"


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {

void CNetServiceTCP::run() {
    const s32 maxe = mConfig->mMaxFetchEvents;
    CEventPoller::SEvent* iEvent = new CEventPoller::SEvent[maxe];
    CNetSession* iContext;
    SContextIO* iAction = 0;
    const u64 tmgap = mWheel.getInterval();
    u64 lastshow = mCurrentTime;
    u64 last = mCurrentTime;
    s32 gotsz = 0;
    mWheel.setCurrent(mCurrentTime);
    u32 action;
    s32 ret;
    for(; mRunning; ) {
        gotsz = mPoller.getEvents(iEvent, maxe, mTimeInterval);
        if(gotsz > 0) {
            mCurrentTime = IAppTimer::getRelativeTime();
            for(u32 i = 0; i < gotsz; ++i) {
                ret = 2;
                if(iEvent[i].mKey < ENET_SESSION_MASK && iEvent[i].mPointer) {
                    iContext = mSessionPool.getSession(iEvent[i].mKey);
                    iAction = APP_GET_VALUE_POINTER(iEvent[i].mPointer, SContextIO, mOverlapped);
                    iAction->mBytes = iEvent[i].mBytes;
                    mTotalReceived += iEvent[i].mBytes;

                    switch(iAction->mOperationType) {
                    case EOP_SEND:
                        ret = iContext->stepSend(*iAction);
                        break;

                    case EOP_RECEIVE:
                        ret = iContext->stepReceive(*iAction);
                        break;

                    case EOP_CONNECT:
                        ret = iContext->stepConnect(*iAction);
                        break;

                    case EOP_DISCONNECT:
                        ret = iContext->stepDisonnect(*iAction);
                        break;

                    default:
                        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::run",
                            "unknown operation: [%u]", iAction->mOperationType);
                        continue;
                    }//switch
                } else {
                    APP_ASSERT(0 == iEvent[i].mPointer);
                    if(ENET_SESSION_MASK == iEvent[i].mKey) {
                        mRunning = false;
                        continue;
                    }
                    action = ENET_CMD_MASK & iEvent[i].mKey;
                    iContext = mSessionPool.getSession(ENET_SESSION_MASK & iEvent[i].mKey);

                    switch(action) {
                    case ENET_CMD_SEND:
                        //AppAtomicDecrementFetch(&mLaunchedSendRequest);
                        ret = -1;
                        break;
                    case ENET_CMD_RECEIVE:
                        ret = iContext->postReceive();
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
                    default:
                        //APP_ASSERT(0);
                        ret = -1;
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
            bool bigerror = false;
            switch(pcode) {
            case WAIT_TIMEOUT:
                mCurrentTime = IAppTimer::getRelativeTime();
                mWheel.update(mCurrentTime);
                last = mCurrentTime;
                if(mCurrentTime - lastshow >= 1000) {
                    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run",
                        "[context:%u/%u][socket:%u/%u]",
                        mSessionPool.getActiveCount(), mSessionPool.getMaxContext(),
                        mClosedSocket, mCreatedSocket);
                    lastshow = mCurrentTime;
                }
                break;

            case ERROR_ABANDONED_WAIT_0: //socket closed
            default:
                IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::run",
                    "invalid overlap, ecode: [%lu]", pcode);
                APP_ASSERT(0);
                bigerror = true;
                break;
            }//switch
            if(bigerror) {
                break;
            }
        }//if

        dispatchBuffers();
    }//for

    delete[] iEvent;
    IAppLogger::log(ELOG_INFO, "CNetServiceTCP::run", "thread exited");
}

//void CNetServiceTCP::postSend() {
//    for(u32 i = 0; i < mConfig->mMaxSendMsg; ) {
//        CBufferQueue::SBuffer* buf = mQueueSend.getHead();
//        if(0 == buf) { break; }
//        if(buf->mSessionCount >= buf->mSessionMax) {
//            ++i;
//            mQueueSend.pop();
//            continue;
//        }
//        for(; buf->mSessionCount < buf->mSessionMax; ++buf->mSessionCount) {
//            mSessionPool[buf->getSessionID(buf->mSessionCount)].postSend();
//            ++i;
//        }
//        mQueueSend.pop();
//    }
//}


bool CNetServiceTCP::clearError() {
    s32 ecode = CNetSocket::getError();
    switch(ecode) {
    case WSA_IO_PENDING:
        return true;
    case ERROR_SEM_TIMEOUT: //hack? 每秒收到5000个以上的Accept时出现
        return true;
    case WAIT_TIMEOUT: //same as WSA_WAIT_TIMEOUT
        break;
    case ERROR_CONNECTION_ABORTED: //服务器主动关闭
    case ERROR_NETNAME_DELETED: //客户端主动关闭连接
    {
        IAppLogger::log(ELOG_INFO, "CNetServiceTCP::clearError",
            "client quit abnormally, [ecode=%u]", ecode);
        //removeClient(pContext);
        return true;
    }
    default:
    {
        IAppLogger::log(ELOG_ERROR, "CNetServiceTCP::clearError",
            "IOCP operation error, [ecode=%u]", ecode);
        return false;
    }
    }//switch

    return false;
}


}//namespace net
}//namespace irr
#endif //APP_PLATFORM_WINDOWS
