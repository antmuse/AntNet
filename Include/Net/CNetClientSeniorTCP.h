#ifndef APP_CNETCLIENTSENIORTCP_H
#define APP_CNETCLIENTSENIORTCP_H


#include "HNetOperationType.h"
#include "irrList.h"
#include "CNetSocket.h"
#include "CEventPoller.h"
#include "INetClientSeniorTCP.h"
#include "CTimerWheel.h"
#include "CNetSession.h"

namespace irr {
class CMutex;
class CThread;

namespace net {

class CNetClientSeniorTCP : public INetClientSeniorTCP {
public:

    CNetClientSeniorTCP();

    virtual ~CNetClientSeniorTCP();

    virtual u32 getReuseRate()override;

    virtual u32 getCreatedSocket()override;

    virtual u32 getTotalSession()override;

    virtual u32 getClosedSocket()override;

    virtual bool start()override;

    virtual bool stop()override;

    virtual INetSession* getSession(INetEventer* receiver) override;

    bool isRunning()const {
        return mRunning;
    }

    virtual u32 getTimeoutInterval()const override {
        return mTimeInterval;
    }

    virtual void setTimeoutInterval(u32 it) override {
        mTimeInterval = it;
    }

    virtual u64 getTotalReceived()const override {
        return mTotalReceived;
    }

    virtual u64 getTotalTime()const override {
        return mCurrentTime - mStartTime;
    }

    //u32 updateTimeWheel();

    /*CTimerWheel& getTimeWheel() {
        return mWheel;
    }*/

protected:

    void run() override;

    void clearError(CNetSession* iContext, SContextIO* iAction);

    void remove(CNetSession* iContext);

    void createContext(u32 max);

private:
#if defined(APP_DEBUG)
    const u32 mMaxContext = 1000;
#else
    const u32 mMaxContext = 10000;
#endif
    bool mRunning;
    u32 mCreatedSocket;
    u32 mClosedSocket;
    u32 mTotalSession;
    u32 mTimeInterval;
    u64 mStartTime;
    u64 mCurrentTime;
    u64 mTotalReceived;     ///<in bytes
    CNetSessionPool mSessionPool;
    CMutex* mMutex;
    CTimerWheel mWheel;
    CThread* mNetThread;
    CEventPoller mPoller;
};

} //namespace net
} //namespace irr

#endif //APP_CNETCLIENTSENIORTCP_H
