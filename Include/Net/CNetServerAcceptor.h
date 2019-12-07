/**
*@file CNetServerAcceptor.h
*Defined the acceptor of TCP net server.
*/

#ifndef APP_CNETSERVERACCEPTOR_H
#define APP_CNETSERVERACCEPTOR_H


#include "irrString.h"
#include "irrArray.h"
#include "IRunnable.h"
#include "CThread.h"
#include "CEventPoller.h"
#include "CNetSocket.h"
#include "CNetService.h"

#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
namespace net {

/**
*@class CNetServerAcceptor
*/
class CNetServerAcceptor : public IRunnable {
public:
    CNetServerAcceptor();

    virtual ~CNetServerAcceptor();

    virtual void run()override;

    bool start(CNetConfig* cfg);

    bool stop();

    s32 send(u32 id, const void* buffer, s32 size);

    CNetServiceTCP* getServer(u32 id)const;

    void setLocalAddress(const CNetAddress& it) {
        mAddressLocal = it;
    }

    const CNetAddress& getLocalAddress() const {
        return mAddressLocal;
    }

    void setMaxAccept(u32 max) {
        mAcceptCount = max;
    }

    void setEventer(u32 id, INetEventer* evt);

    void setEventer(INetEventer* evt) {
        mReceiver = evt;
    }


protected:
    struct SContextWaiter {
        s8 mCache[64];      //Must no less than (2*(sizeof(sockaddr_in) + 16))
        CNetSocket mSocket;
        SContextIO* mContextIO;

        SContextWaiter();
        ~SContextWaiter();
        bool reset();
    };



    bool clearError();

    bool initialize();

    void removeAll();


    /**
    *@brief Post an accpet.
    *@param iContext Accpet context
    *@return True if successed, else false.
    */
    bool postAccept(SContextWaiter* iContext);


    /**
    *@brief Accpet a incoming client.
    *@param iContext Accpet context
    *@return True if successed, else false.
    */
    bool stepAccpet(SContextWaiter* iContext);

    bool postAccept();

    CNetServiceTCP* createServer();

    void removeAllServer();

    void addServer(CNetServiceTCP* it);

    bool removeServer(CNetServiceTCP* it);


private:
    volatile bool mRunning;                ///<True if started, else false
    u32 mAcceptCount;
    u32 mCurrent;
    CEventPoller mPoller;
    CThread* mThread;						///<All workers
    CNetSocket mListener;					///<listen socket's context
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;
    core::array<SContextWaiter*>  mAllWaiter;
    core::array<CNetServiceTCP*>  mAllService;
    void* mFunctionAccept;
    void* mFunctionAcceptSockAddress;
    INetEventer* mReceiver;
    CNetConfig* mConfig;
};


}// end namespace net
}// end namespace irr


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace irr {
namespace net {

/**
*@class CNetServerAcceptor
*/
class CNetServerAcceptor : public IRunnable {
public:
    CNetServerAcceptor();

    virtual ~CNetServerAcceptor();

    virtual void run()override;

    bool start(CNetConfig* cfg);

    bool stop();

    s32 send(u32 id, const void* buffer, s32 size);

    void setLocalAddress(const CNetAddress& it) {
        mAddressLocal = it;
    }

    const CNetAddress& getLocalAddress() const {
        return mAddressLocal;
    }

    void setMaxAccept(u32 max) {
        mAcceptCount = max;
    };

    void setEventer(INetEventer* evt) {
        mReceiver = evt;
    }

    void addServer(CNetServiceTCP* it);

    bool removeServer(CNetServiceTCP* it);

private:
    bool clearError();

    bool initialize();

    void removeAll();

    bool stepAccpet(CNetSocket& sock);

    bool postAccept();

    CNetServiceTCP* createServer();

    void removeAllServer();

    volatile bool mRunning;                    ///<True if started, else false
    u32 mAcceptCount;
    u32 mCurrent;
    CEventPoller mPoller;
    INetEventer* mReceiver;
    CNetConfig* mConfig;
    CThread* mThread;						    ///<All workers
    CNetSocket mListener;						///<listen socket's context
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;
    core::array<CNetServiceTCP*>  mAllService;
};

}// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_LINUX

#endif //APP_CNETSERVERACCEPTOR_H
