/**
*@file CNetServerSeniorTCP.h
*Defined CNetServerSeniorTCP, SContextIO, SClientContext
*/

#ifndef APP_CNETSERVERSENIORTCP_H
#define APP_CNETSERVERSENIORTCP_H


#include "irrString.h"
#include "irrArray.h"
#include "irrList.h"
#include "IRunnable.h"
#include "CThread.h"
#include "CEventPoller.h"
#include "CNetSocket.h"
#include "CTimerWheel.h"
#include "CNetSession.h"

#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {
class INetEventer;
struct SContextIO;

/**
*@class CNetServerSeniorTCP
*/
class CNetServerSeniorTCP : public IRunnable {
public:
    CNetServerSeniorTCP(CNetConfig* cfg);

    virtual ~CNetServerSeniorTCP();

    virtual void run()override;

    bool start();

    bool stop();

    CNetSession* addSession(CNetSocket& sock, const CNetAddress& remote, 
        const CNetAddress& local, INetEventer* evter);

protected:
    bool clearError();

    void remove(CNetSession* iContext);

private:

    bool mRunning;
    u32 mCreatedSocket;
    u32 mClosedSocket;      ///closed socket count
    u32 mTotalSession;      ///<launched session count
    u32 mTimeInterval;
    u64 mStartTime;
    u64 mCurrentTime;
    u64 mTotalReceived;
    CTimerWheel mWheel;
    CEventPoller mPoller;
    CMutex mMutex;
    CNetSessionPool mSessionPool;
    CThread* mThread;
    INetEventer* mReceiver;
    //CMemoryHub mMemHub;
    CNetConfig* mConfig;
};


}// end namespace net
}// end namespace irr


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace irr {
namespace net {
class INetEventer;
class CNetSession;

/**
*@class CNetServerSeniorTCP
*/
class CNetServerSeniorTCP : public IRunnable {
public:
    CNetServerSeniorTCP();

    virtual ~CNetServerSeniorTCP();

    virtual void run()override;

    bool start();

    bool stop();

    void setMaxClients(u32 max) {
        mMaxContext = max;
    };

    virtual u32 getClientCount()const;

    CNetSession* addSession(CNetSocket& sock, const CNetAddress& remote, const CNetAddress& local);

protected:
    bool clearError();

    void remove(CNetSession* iContext);

    void createContext(u32 max);

    void deleteContext();

private:

    bool mRunning;
    u32 mMaxContext;
    u32 mCreatedSession;
    u32 mCreatedSocket;
    u32 mClosedSocket;      ///closed socket count
    u32 mTotalSession;      ///<launched session count
    u32 mTimeInterval;
    u64 mStartTime;
    u64 mCurrentTime;
    u64 mTotalReceived;
    CTimerWheel mWheel;
    CEventPoller mPoller;
    CMutex mMutex;
    CNetSession* mAllContext;
    core::list<u32> mIdleSession;
    CThread* mThread;
    INetEventer* mReceiver;
    CNetAddress mAddressLocal;
    //CMemoryHub mMemHub;
};


}// end namespace net
}// end namespace irr

#endif //APP_PLATFORM_LINUX

#endif //APP_CNETSERVERSENIORTCP_H
