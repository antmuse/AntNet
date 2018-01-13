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
#include "CNetServerSeniorTCP.h"

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

    bool start();

    bool stop();

    void setLocalAddress(const SNetAddress& it) {
        mAddressLocal = it;
    }

    const SNetAddress& getLocalAddress() const {
        return mAddressLocal;
    }

    void setMaxAccept(u32 max) {
        mAcceptCount = max;
    };

    void addServer(CNetServerSeniorTCP* it);

    bool removeServer(CNetServerSeniorTCP* it);

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


protected:
    bool postAccept();
    CNetServerSeniorTCP* createServer();
    void removeAllServer();

private:
    bool mRunning;                                  ///<True if started, else false
    u32 mAcceptCount;
    u32 mCurrent;
    CEventPoller mPoller;
    CThread* mThread;						        ///<All workers
    CNetSocket mListener;						///<listen socket's context
    SNetAddress mAddressRemote;
    SNetAddress mAddressLocal;
    core::array<SContextWaiter*>  mAllWaiter;
    core::array<CNetServerSeniorTCP*>  mAllService;
    void* mFunctionAccept;
    void* mFunctionAcceptSockAddress; 
};


}// end namespace net
}// end namespace irr


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

namespace irr {
namespace net {
struct SContextIO;
struct CNetSession;
class CNetSession;


/**
*@class CNetServerAcceptor
*/
class CNetServerAcceptor : public INetServer, public IRunnable {
public:
    CNetServerAcceptor();
    virtual ~CNetServerAcceptor();

    virtual ENetNodeType getType() const {
        return ENET_TCP_SERVER;
    }
    virtual void setNetEventer(INetEventer* it) {
        mReceiver = it;
    }
    virtual bool start();
    virtual bool stop();

    virtual void run()override;

    virtual void setLocalAddress(const SNetAddress& it)override {
        mAddressLocal = it;
    }

    virtual const SNetAddress& getLocalAddress() const override {
        return mAddressLocal;
    }

    virtual void setMaxClients(u32 max) {
        mMaxClientCount = max;
    };

    virtual u32 getClientCount() const {
        return mAllClient.size();
    }

    void addClient(CNetSession* iContextSocket);

    void removeClient(CNetSession* iContextSocket);

    void removeAllClient();

    bool clearError(CNetSession* pContext);

    /**
    *@brief Request an accept action.
    *@param iContextIO,
    *@return True if request finished, else false.
    */
    bool postAccept(SContextIO* iContextIO);

    /**
    *@brief Request a receive action.
    *@param iContextIO,
    *@return True if request finished, else false.
    */
    bool postReceive(CNetSession* iContextSocket);

    /**
    *@brief Request a send action.
    *@param iContextIO,
    *@return True if request finished, else false.
    */
    bool postSend(CNetSession* iContextSocket);

    /**
    *@brief Go to next step when got the IO finished info: accpet.
    *@param iContextSocket,
    *@param iContextIO,
    *@return True if next step finished, else false.
    */
    bool stepAccpet(SContextIO* iContextIO);

    /**
    *@brief Go to next step when got the IO finished info: receive.
    *@param iContextSocket,
    *@param iContextIO,
    *@return True if next step finished, else false.
    */
    bool stepReceive(CNetSession* iContextSocket);

    /**
    *@brief Go to next step when got the IO finished info: send.
    *@param iContextSocket,
    *@param iContextIO,
    *@return True if next step finished, else false.
    */
    bool stepSend(CNetSession* iContextSocket);


    virtual void setThreads(u32 max) {
        mThreadCount = max;
    }

private:
    bool initialize();
    void removeAll();

    SNetAddress mAddressLocal;
    bool mRunning;                                  ///<True if server started, else false
    u32 mThreadCount;
    u32 mMaxClientCount;
    CEventPoller mEpollFD;
    CMutex mMutex;
    core::array<CNetSession*>  mAllClient;          ///<All the clients's socket context
    core::array<CThread*>  mAllWorker;                  ///<All workers
    SContextIO* mAccpetIO;
    CNetSocket mListener;
    INetEventer* mReceiver;
};

}// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_LINUX

#endif //APP_CNETSERVERACCEPTOR_H
