#include "CThreadPool.h"
#include "IAppLogger.h"

namespace irr {

CThreadPool::CThreadPool(u32 iThreadCount) :
    mWaitingTasks(0),
    mActiveCount(0),
    mWorker(0),
    mTaskListTail(0),
    mThreadCount(iThreadCount),
    mStatus(ESTATUS_STOPED),
    mMaxTasks(0) {
    mTaskListTail = &mTaskListHead;
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

    SThreadTask* nd;
    while(mTaskListHead.mNext) {
        nd = mTaskListHead.mNext;
        mTaskListHead.mNext = mTaskListHead.mNext->mNext;
        delete nd;
        --mWaitingTasks;
    }
    mTaskListHead.mNext = 0;
    mTaskListTail = &mTaskListHead;
    APP_ASSERT(0 == mWaitingTasks);
}


void CThreadPool::run() {
    mMutex.lock();
    ++mActiveCount;
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::run", "thread start: %u", CThread::getCurrentThread()->getID());
    mMutex.unlock();

    SThreadTask* iTask = 0;
    for(; true;) {
        mMutex.lock();
        //@note: as we are in thread pool's loop yet,
        //so it's needless to avoid spurious wakeup here.
        if(mTaskListHead.mNext) {
            iTask = mTaskListHead.mNext;
            mTaskListHead.mNext = mTaskListHead.mNext->mNext;
            --mWaitingTasks;
            if(0 == mTaskListHead.mNext) {
                APP_ASSERT(0 == mWaitingTasks);
                APP_ASSERT(mTaskListTail == iTask);
                mTaskListTail = &mTaskListHead;
            }
        } else {
            mCondition.wait(mMutex);//mMutex locked
        }
        mMutex.unlock();

        if(ESTATUS_STOPED == mStatus) {
            //ignore leftover tasks and exit
            break;
        }
        if(mWaitingTasks > 0) {
            mCondition.notify();
        }
        if(iTask) {
            (*iTask)();
            delete iTask;
            iTask = 0;
        }
        if(ESTATUS_JOINING == mStatus && 0 == mWaitingTasks) {
            //finished all tasks then exit
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
    creatThread(mThreadCount);
    while(mActiveCount < mThreadCount) {
        IAppLogger::log(ELOG_CRITICAL, "CThreadPool::start",
            "waiting all threads start: %u/%u", mActiveCount, mThreadCount);
        CThread::sleep(10);
    }
}


void CThreadPool::stop() {
    if((ESTATUS_JOINING | ESTATUS_STOPED) & mStatus) {
        return;
    }
    mStatus = ESTATUS_STOPED;
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::stop",
        "[active=%u],[threads=%u],[tasks=%u]",
        mActiveCount, mThreadCount, mWaitingTasks);
    while(mActiveCount > 0) {
        CThread::sleep(10);
        mCondition.notifyAll();
    }
    removeAll();
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::stop",
        "threads[%u], tasks[%u]",
        mThreadCount, mWaitingTasks);
}


void CThreadPool::join() {
    if((ESTATUS_JOINING | ESTATUS_STOPED) & mStatus) {
        return;
    }
    mStatus = ESTATUS_JOINING;
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::join",
        "[active=%u],[threads=%u],[tasks=%u]",
        mActiveCount, mThreadCount, mWaitingTasks);
    while(mActiveCount > 0) {
        CThread::sleep(10);
        mCondition.notify();
    }
    removeAll();
    IAppLogger::log(ELOG_CRITICAL, "CThreadPool::join",
        "threads[%u], tasks[%u]",
        mThreadCount, mWaitingTasks);
}


bool CThreadPool::start(AppCallable iFunc, void* iData/* = 0*/) {
    if(!iFunc || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    bool ret = true;
    CAutoLock ak(mMutex);
    if(mMaxTasks > 0 && mWaitingTasks >= mMaxTasks) {
        ret = false;
    } else {
        mTaskListTail->mNext = new SThreadTask(iFunc, iData);
        mTaskListTail = mTaskListTail->mNext;
        ++mWaitingTasks;
    }
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
    CAutoLock ak(mMutex);
    if(mMaxTasks > 0 && mWaitingTasks) {
        ret = false;
    } else {
        mTaskListTail->mNext = new SThreadTask(it);
        mTaskListTail = mTaskListTail->mNext;
        ++mWaitingTasks;
    }
    if(ret) {
        mCondition.notify();
    }
    return ret;
}


}//irr
