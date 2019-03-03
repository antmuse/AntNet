/**
*@file CThreadPool.h
*@brief This file defined a pool of threads.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_CTHREADPOOL_H
#define APP_CTHREADPOOL_H

#include "irrList.h"
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

    bool addTask(AppCallable iFunc, void* iData = 0);

    bool addTask(IRunnable* it);

    /**
    * @bire A threadpool only have one Sole-Task.
    */
    bool addSoleTask(IRunnable* it);
    bool addSoleTask(AppCallable iFunc, void* iData = 0);

    u32 getMaxThreads()const {
        return mThreadCount;
    }

    u32 getWaitingTasks()const {
        return mWaitingTasks;
    }
    
private:
    enum {
        ESTATUS_STOPED = 1,
        ESTATUS_RUNNIG = 1 << 1,
        ESTATUS_JOINING = 1 << 2
    };

    volatile u16 mActiveCount;
    volatile u16 mStatus;
    volatile u32 mWaitingTasks;
    u32 mThreadCount;
    u32 mMaxTasks;          ///<0: disable limit, else limit the max tasks saved in list. default: 0
    CMutex mMutex;          ///<note: mutex type PTHREAD_MUTEX_TIMED_NP,PTHREAD_MUTEX_ADAPTIVE_NP
    CCondition mCondition;
    SThreadTask mTaskListHead;
    SThreadTask* mTaskListTail;
    CThread** mWorker;


    CThreadPool() { }
    CThreadPool(const CThreadPool& it) = delete;
    CThreadPool& operator=(const CThreadPool& it) = delete;

    void creatThread(u32 iCount);

    void removeAll();
};

}//irr


#endif	/* APP_CTHREADPOOL_H */