/**
*@file CNetServiceTCP.h
*Defined CNetServiceTCP, SContextIO, SClientContext
*/

#ifndef APP_CNETSERVICE_H
#define APP_CNETSERVICE_H


#include "irrString.h"
#include "irrArray.h"
#include "irrList.h"
#include "CThreadPool.h"
#include "CEventPoller.h"
#include "CNetSocket.h"
#include "CTimerWheel.h"
#include "CNetSessionPool.h"

namespace irr {
namespace net {
class INetEventer;
struct SContextIO;

/**
*@class CNetServiceTCP
*/
class CNetServiceTCP : public IRunnable {
public:
    CNetServiceTCP();

    virtual ~CNetServiceTCP();

    virtual void run()override;

    static void threadPoolCall(void* it);

    bool start(CNetConfig* cfg);

    bool stop();

    u32 getID() const {
        return mID;
    }
    void setID(u32 id) {
        mID = id & (((u32) ENET_SERVER_MASK) >> ENET_SESSION_BITS);
    }

    CNetConfig* setConfig(CNetConfig* cfg);

    void setEventer(u32 id, INetEventer* evt);

    u32 receive(CNetSocket& sock, const CNetAddress& remote,
        const CNetAddress& local, INetEventer* evter);

    u32 connect(const CNetAddress& remote, INetEventer* it);

    bool disconnect(u32 id);

    s32 send(u32 id, const void* buffer, s32 size);

    s32 send(const u32* id, u16 maxUID, const void* buffer, s32 size);

    CEventPoller& getPoller() {
        return mPoller;
    }

    CMemoryHub& getMemoryHub() {
        return *mMemHub;
    }

    void setMemoryHub(CMemoryHub* it) {
        if(it && it != mMemHub) {
            it->grab();
            if(mMemHub) {
                mMemHub->drop();
            }
            mMemHub = it;
            mQueueSend.setMemoryHub(mMemHub);
            mQueueEvent.setMemoryHub(mMemHub);
        }
    }

    void addNetEvent(CNetSession& session);

    CEventQueue& getEventQueue() {
        return mQueueEvent;
    }

    CNetSession& getSession(u32 idx) {
        return mSessionPool[idx];
    }

protected:
    bool clearError();
    void remove(CNetSession* iContext);
    bool activePoller(u32 cmd, u32 id = 0);
    void dispatchBuffers();
    void clearBuffers();

private:
    volatile bool mRunning;
    u32 mID;
    u32 mCreatedSocket;
    u32 mClosedSocket;      ///closed socket count
    u32 mTimeInterval;
    u64 mStartTime;
    u64 mCurrentTime;
    u64 mTotalReceived;
    CTimerWheel mWheel;
    CEventPoller mPoller;
    CMutex mMutex;
    CNetSessionPool mSessionPool;
    CThread* mThread;
    CThreadPool* mThreadPool;
    INetEventer* mReceiver;
    CBufferQueue mQueueSend;
    CEventQueue mQueueEvent;//processecd by worker threads
    CMemoryHub* mMemHub;
    CNetConfig* mConfig;

    CNetServiceTCP(const CNetServiceTCP& it) = delete;
    CNetServiceTCP& operator=(const CNetServiceTCP& it) = delete;

#if defined(APP_PLATFORM_WINDOWS)

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

#endif //APP_PLATFORM_LINUX
};


}// end namespace net
}// end namespace irr

#endif //APP_CNETSERVICE_H
