#include "CThreadPool.h"
#include "IAppLogger.h"

namespace irr {


    void CThreadPool::CThreadWorker::run() {
        for(; mRunning; ) {
            if(mTask) {
                mTask->run();
                mTask = 0;
                mPool->onTask(this);
            } else if(mCallTask.mCallback) {
                mCallTask.mCallback(mCallTask.mData);
                mCallTask.mCallback = 0;
                mCallTask.mData = 0;
                mPool->onTask(this);
            } else {
                mThread.sleep(100);
            }
        }//for
    }



    CThreadPool::CThreadPool(u32 iThreadCount) : mThreadCount(iThreadCount),
        mRunning(false),
        mMaxTasks(0){
            creatThread(iThreadCount);
    }


    CThreadPool::~CThreadPool() {
        removeAll();
    }


    void CThreadPool::creatThread(u32 iCount) {
        mIdleWorker.reallocate(iCount, false);
        mBusyWorker.reallocate(iCount, false);
        mBusyWorker.set_used(iCount);
        memset(mBusyWorker.pointer(), 0, iCount * sizeof(CThreadWorker*));
        for(u32 i = 0; i < iCount; ++i) {
            mIdleWorker.push_back(new CThreadWorker(this, i));
            mIdleWorker[i]->start();
        }
        //IAppLogger::log(ELOG_INFO, "CThreadPool::creatThread", "created thereads success, total: [%d]",iCount);
    }


    void CThreadPool::removeAll() {
        mHoldTasks.clear();
        mHoldCallTasks.clear();

        for(u32 i = 0; i < mBusyWorker.size(); ++i) {
            if(mBusyWorker[i]) {
                mBusyWorker[i]->stop();
                mBusyWorker[i] = 0;
            }
        }
        for(u32 i = 0; i < mIdleWorker.size(); ++i) {
            mIdleWorker[i]->stop();
            delete mIdleWorker[i];
        }
        mIdleWorker.set_used(0);
    }


    void CThreadPool::onTask(CThreadWorker* wk) {
        mMutex.lock();
        if(mHoldTasks.size()>0) {
            core::list<IRunnable*>::Iterator it = mHoldTasks.begin();
            wk->setTask(*it);
            mHoldTasks.erase(it);
            APP_LOG(ELOG_CRITICAL, "CThreadPool::onTask", "Worker[%u] got task ----------- total idle [%u]", wk->getID(), mIdleWorker.size());
        }	else if(mHoldCallTasks.size()>0) {
            core::list<SCallbackData>::Iterator it = mHoldCallTasks.begin();
            wk->setTask((*it).mCallback, (*it).mData);
            mHoldCallTasks.erase(it);
            APP_LOG(ELOG_CRITICAL, "CThreadPool::onTask", "Worker[%u] got call task ----------- total idle [%u]", wk->getID(), mIdleWorker.size());
        }	else {
            APP_LOG(ELOG_CRITICAL, "CThreadPool::onTask", "Worker[%u] back ----------- total idle [%u]", wk->getID(), mIdleWorker.size());
            mIdleWorker.push_back(wk);
            mBusyWorker[wk->getID()] = 0;
        }
        mMutex.unlock();
    }


    void CThreadPool::start() {
        if(mRunning) {
            return;
        }
        mRunning = true;
    }


    void CThreadPool::stop() {
        if(!mRunning) {
            return;
        }
        mRunning = false;
        IAppLogger::log(ELOG_CRITICAL, "CThreadPool::stop", "sum[%u] idle[%u] busy[%u]  stopping...", mThreadCount, mIdleWorker.size(), mHoldTasks.size());
        while(mIdleWorker.size() < mThreadCount) {
            CThread::sleep(200);
            APP_LOG(ELOG_CRITICAL, "CThreadPool::stop", "sum[%u] idle[%u] calltask[%u]", mThreadCount, mIdleWorker.size(), mHoldCallTasks.size());
        }
        mHoldTasks.clear();
        mHoldCallTasks.clear();
        IAppLogger::log(ELOG_CRITICAL, "CThreadPool::stop", "all threads stoped success");
    }


    bool CThreadPool::start(AppCallable iFunc, void* iData/* = 0*/) {
        if(!iFunc || !mRunning) {
            return false;
        }

        mMutex.lock();
        bool ret = true;

        if(mIdleWorker.size()>0) {
            CThreadWorker* wk = mIdleWorker.getLast();
            mIdleWorker.set_used(mIdleWorker.size() - 1);
            mBusyWorker[wk->getID()] = wk;
            wk->setTask(iFunc, iData);
            APP_LOG(ELOG_CRITICAL, "CThreadPool::start", "Worker[%u] got call task ----------- total idle [%u]", wk->getID(), mIdleWorker.size());
        } else if(mMaxTasks>0 && mHoldCallTasks.size() >= mMaxTasks) {
            ret = false;
        } else {
            SCallbackData scd(iFunc, iData);
            mHoldCallTasks.push_back(scd);
        }


        mMutex.unlock();
        return ret;
    }


    bool CThreadPool::start(IRunnable* it) {
        if(!it || !mRunning) {
            return false;
        }
        mMutex.lock();
        bool ret = true;

        if(mIdleWorker.size()) {
            CThreadWorker* wk = mIdleWorker.getLast();
            mIdleWorker.set_used(mIdleWorker.size() - 1);
            mBusyWorker[wk->getID()] = wk;
            wk->setTask(it);
            APP_LOG(ELOG_CRITICAL, "CThreadPool::start", "Worker[%u] got task ----------- total idle [%u]", wk->getID(), mIdleWorker.size());
        } else if(mMaxTasks>0 && mHoldTasks.size() >= mMaxTasks) {
            ret = false;
        } else {
            mHoldTasks.push_back(it);
        }

        mMutex.unlock();

        return ret;
    }


}//irr
