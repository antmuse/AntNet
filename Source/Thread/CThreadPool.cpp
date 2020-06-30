#include "CThreadPool.h"
#include "CLogger.h"
#include "HAtomicOperator.h"

namespace app {
#if defined(APP_DEBUG)
static s32 G_ENQUEUE_COUNT = 0;
static s32 G_DEQUEUE_COUNT = 0;
#endif

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
    //CLogger::log(ELOG_INFO, "CThreadPool::creatThread", "created thereads success, total: [%d]",iCount);
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
    CLogger::log(ELOG_CRITICAL, "CThreadPool::run", "thread start: %u", CThread::getCurrentThread()->getID());
    mMutex.unlock();

    SThreadTask* iTask = 0;
    bool deleteTask = true;

    while(ESTATUS_STOPED != mStatus) {
        mMutex.lock();
        while(ESTATUS_STOPED != mStatus) {//@note: avoid spurious wakeup in a loop.
            if(mTaskListHead.mNext) {
                iTask = mTaskListHead.mNext;
                mTaskListHead.mNext = mTaskListHead.mNext->mNext;
                if(0 == mTaskListHead.mNext) {
                    APP_ASSERT(1 == mWaitingTasks);
                    APP_ASSERT(mTaskListTail == iTask);
                    mTaskListTail = &mTaskListHead;
                }
                if(--mWaitingTasks > 0) {
                    mCondition.notify();
                }
                break;
            } 
            if(mTaskListHead.mCount > 0) {
                iTask = &mTaskListHead;
                --mTaskListHead.mCount;
                deleteTask = false;
                if(--mWaitingTasks > 0) {
                    mCondition.notify();
                }
                break;
            }
            //mutex is unlocked when waiting and will be locked when awaked.
            mCondition.wait(mMutex);
            if(ESTATUS_JOINING == mStatus && 0 == mWaitingTasks) {
                mStatus = ESTATUS_STOPED;//finished all tasks then exit
            }
        }//while
        mMutex.unlock();

        if(iTask) {
#if defined(APP_DEBUG)
            AppAtomicIncrementFetch(&G_DEQUEUE_COUNT);
#endif
            (*iTask)(); //executed task
            if(deleteTask) {
                delete iTask;
            }
            deleteTask = true;
            iTask = 0;
        }
    }//for

    mMutex.lock();
    --mActiveCount;
    mMutex.unlock();
    CLogger::log(ELOG_CRITICAL, "CThreadPool::run", "thread quit: %u", CThread::getCurrentThread()->getID());
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
        CLogger::log(ELOG_CRITICAL, "CThreadPool::start",
            "waiting all threads start: %u/%u", mActiveCount, mThreadCount);
        CThread::sleep(10);
    }
}


void CThreadPool::stop() {
    if((ESTATUS_JOINING | ESTATUS_STOPED) & mStatus) {
        return;
    }
    mStatus = ESTATUS_STOPED;
    CLogger::log(ELOG_CRITICAL, "CThreadPool::stop",
        "[active=%u],[threads=%u],[tasks=%u]",
        mActiveCount, mThreadCount, mWaitingTasks);
    while(mActiveCount > 0) {
        mCondition.notifyAll();
        CThread::sleep(20);
    }
    removeAll();
    CLogger::log(ELOG_CRITICAL, "CThreadPool::stop",
        "threads[%u], tasks[%u]",
        mThreadCount, mWaitingTasks);
}


void CThreadPool::join() {
    if((ESTATUS_JOINING | ESTATUS_STOPED) & mStatus) {
        return;
    }
    mStatus = ESTATUS_JOINING;
    CLogger::log(ELOG_CRITICAL, "CThreadPool::join",
        "[active=%u],[threads=%u],[tasks=%u]",
        mActiveCount, mThreadCount, mWaitingTasks);
    while(mActiveCount > 0) {
        mCondition.notify();
        CThread::sleep(20);
    }
    APP_ASSERT(G_ENQUEUE_COUNT == G_DEQUEUE_COUNT);
    removeAll();
    CLogger::log(ELOG_CRITICAL, "CThreadPool::join",
        "threads[%u], tasks[%u]",
        mThreadCount, mWaitingTasks);
}


bool CThreadPool::addTask(AppCallable iFunc, void* iData/* = 0*/) {
    if(!iFunc || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    bool ret = false;
    CAutoLock ak(mMutex);

    if(0 == mMaxTasks || mWaitingTasks < mMaxTasks) {
        ret = true;
        mTaskListTail->mNext = new SThreadTask(iFunc, iData);
        mTaskListTail = mTaskListTail->mNext;
        ++mWaitingTasks;
        mCondition.notify();
    }
    return ret;
}


bool CThreadPool::addTask(IRunnable* it) {
    if(!it || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    bool ret = false;
    CAutoLock ak(mMutex);

    if(0 == mMaxTasks || mWaitingTasks < mMaxTasks) {
        ret = true;
        mTaskListTail->mNext = new SThreadTask(it);
        mTaskListTail = mTaskListTail->mNext;
        ++mWaitingTasks;
        mCondition.notify();
    }
    return ret;
}


bool CThreadPool::addSoleTask(AppCallable iFunc, void* iData/* = 0*/) {
    if(!iFunc || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    CAutoLock ak(mMutex);

    if(0 == mTaskListHead.mCount) {//init task
        mTaskListHead.setTarget(iFunc, iData);
    } else if(iFunc == mTaskListHead.mTarget.mCallFunction.mCallback) {
        ++mTaskListHead.mCount;
    } else {
        return false;
    }

#if defined(APP_DEBUG)
    AppAtomicIncrementFetch(&G_ENQUEUE_COUNT);
#endif
    ++mWaitingTasks;
    mCondition.notify();
    return true;
}


bool CThreadPool::addSoleTask(IRunnable* it) {
    if(!it || ESTATUS_RUNNIG != mStatus) {
        return false;
    }
    CAutoLock ak(mMutex);

    if(0 == mTaskListHead.mCount) {//init task
        mTaskListHead = it;
    } else if(it == mTaskListHead.mTarget.mCaller) {
        ++mTaskListHead.mCount;
    } else {
        return false;
    }

    ++mWaitingTasks;
    mCondition.notify();
    return true;
}

}//app
