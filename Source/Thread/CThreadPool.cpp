#include "CThreadPool.h"
#include "IAppLogger.h"

namespace irr {

CThreadPool::CThreadPool(u32 iThreadCount) :
    mActiveCount(0),
    mWorker(0),
    mThreadCount(iThreadCount),
    mStatus(ESTATUS_STOPED),
    mMaxTasks(0) {
}


CThreadPool::~CThreadPool() {
    stop();
}


void CThreadPool::creatThread(u32 iCount) {
    mWorker = new CThread*[iCount];
    ::memset(mWorker, 0, iCount * sizeof(CThread*));
    for(u32 i = 0; i < iCount; ++i) {
        mWorker[i] = (new CThread());
        mWorker[i]->start(*this);
    }
    //IAppLogger::log(ELOG_INFO, "CThreadPool::creatThread", "created thereads success, total: [%d]",iCount);
}


void CThreadPool::removeAll() {
    for(u32 i = 0; i < mThreadCount; ++i) {
        mWorker[i]->join();
        delete mWorker[i];
        //mWorker[i] = 0;
    }
    delete mWorker;
    mWorker = 0;
    mHoldTasks.clear();
    mHoldCallTasks.clear();
}


void CThreadPool::run() {
    IRunnable* iTask = 0;
    SCallbackData iCallTask;
    iCallTask.mCallback = 0;
    iCallTask.mData = 0;
    u32 leftover = 0; //leftover tasks
    mMutex.lock();
    ++mActiveCount;    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::run", "thread start: %u", CThread::getCurrentThread()->getID());
    mMutex.unlock();

    for(; true;) {
        mMutex.lock();//lock
        //for(; true;) //antmuse: as we are in thread pool's loop yet, needless to avoid spurious wakeup
        if(mHoldTasks.size() > 0) {
            core::list<IRunnable*>::Iterator it = mHoldTasks.begin();
            iTask = (*it);
            mHoldTasks.erase(it);
        } else if(mHoldCallTasks.size() > 0) {
            core::list<SCallbackData>::Iterator it = mHoldCallTasks.begin();
            iCallTask.mCallback = (*it).mCallback;
            iCallTask.mData = (*it).mData;
            mHoldCallTasks.erase(it);
        } else {
            mCondition.wait(mMutex);
        }
        leftover = mHoldCallTasks.size() + mHoldTasks.size();
        mMutex.unlock();//unlock

        if(ESTATUS_STOPED == mStatus) {//ignore leftover tasks and exit
            break;
        }
        if(leftover > 0) {
            mCondition.notify();
        }
        if(iTask) {
            iTask->run();
            iTask = 0;
        } else if(iCallTask.mCallback) {
            iCallTask.mCallback(iCallTask.mData);
            iCallTask.mCallback = 0;
            iCallTask.mData = 0;
        }
        if(ESTATUS_JOINING == mStatus && 0 == leftover) {//finished all tasks then exit
            break;
        }
    }//for

    mMutex.lock();
    --mActiveCount;
    mMutex.unlock();
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::run", "thread quit: %u", CThread::getCurrentThread()->getID());
}


void CThreadPool::start() {
    if(ESTATUS_RUNNIG == mStatus) {
        return;
    }
    mStatus = ESTATUS_RUNNIG;
    mMutex.lock();
    mActiveCount = 0;
    mMutex.unlock();
    creatThread(mThreadCount);    while(mActiveCount<mThreadCount){        IAppLogger::log(ELOG_CRITICAL, "CThreadPool::start", "waiting all threads start: %u", mActiveCount);      CThread::sleep(10);    }
}


void CThreadPool::stop() {
    if((ESTATUS_JOINING | ESTATUS_STOPED) & mStatus) {
        return;
    }
    mStatus = ESTATUS_STOPED;
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::stop", "[active=%u],[threads=%u],[tasks=%u]",
        mActiveCount,mThreadCount, mHoldTasks.size() + mHoldCallTasks.size());
    while(mActiveCount > 0) {
        CThread::sleep(10);
        mCondition.notifyAll();
    }
    removeAll();
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::stop", "threads[%u], tasks[%u]", mThreadCount, mHoldTasks.size() + mHoldCallTasks.size());
}


void CThreadPool::join() {
    if((ESTATUS_JOINING | ESTATUS_STOPED) & mStatus) {
        return;
    }
    mStatus = ESTATUS_JOINING;
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::join", "[active=%u],[threads=%u],[tasks=%u]",
        mActiveCount, mThreadCount, mHoldTasks.size() + mHoldCallTasks.size());
    while(mActiveCount > 0) {
        CThread::sleep(10);
        mCondition.notify();
    }
    removeAll();
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::join", "threads[%u], tasks[%u]", mThreadCount, mHoldTasks.size() + mHoldCallTasks.size());
}


bool CThreadPool::start(AppCallable iFunc, void* iData/* = 0*/) {
    if(!iFunc || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    bool ret = true;

    mMutex.lock();

    if(mMaxTasks > 0 && mHoldCallTasks.size() >= mMaxTasks) {
        ret = false;
    } else {
        SCallbackData scd(iFunc, iData);
        mHoldCallTasks.push_back(scd);
    }

    mMutex.unlock();
    if(ret) {
        mCondition.notify();
    }
    return ret;
}


bool CThreadPool::start(IRunnable* it) {
    if(!it || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    bool ret = true;
    mMutex.lock();
    if(mMaxTasks > 0 && mHoldTasks.size() >= mMaxTasks) {
        ret = false;
    } else {
        mHoldTasks.push_back(it);
    }

    mMutex.unlock();
    if(ret) {
        mCondition.notify();
    }
    return ret;
}


}//irr
