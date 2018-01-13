/**
*@file CThreadPool.h
*@brief This file defined a pool of threads.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_CTHREADPOOL_H
#define APP_CTHREADPOOL_H

#include "irrList.h"
#include "IRunnable.h"
#include "CThread.h"
#include "CCondition.h"

namespace irr {

/**
*@class CThreadPool
*@brief A thread pool work on Windows, Linux, and Android.
*/
class CThreadPool : public IRunnable {
public:
    CThreadPool(u32 iThreadCount);

    virtual ~CThreadPool();

    virtual void run()override;

    /**
    *@param it 0: no limit
    */
    void setMaxHoldTasks(u32 it) {
        mMaxTasks = it;
    }

    void start();

    void stop();

    void join();

    bool start(AppCallable iFunc, void* iData = 0);

    bool start(IRunnable* it);

    u32 getMaxThreads()const {
        return mThreadCount;
    }


private:
    enum {
        ESTATUS_STOPED = 1,
        ESTATUS_RUNNIG = 1<<1,
        ESTATUS_JOINING = 1<<2,
    };
    CThreadPool() {
    }

    void creatThread(u32 iCount);

    void removeAll();

    u16 mActiveCount;
    u16 mStatus;
    u32 mThreadCount;
    u32 mMaxTasks;          ///<0: disable limit, else limit the max tasks saved in list. default: 0
    CMutex mMutex;          ///<note: mutex type PTHREAD_MUTEX_TIMED_NP,PTHREAD_MUTEX_ADAPTIVE_NP
    CCondition mCondition;
    core::list<IRunnable*> mHoldTasks;
    core::list<SCallbackData> mHoldCallTasks;
    CThread** mWorker;
};

}//irr


#endif	/* APP_CTHREADPOOL_H */