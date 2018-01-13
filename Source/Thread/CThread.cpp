#include "CThread.h"
#include "IRunnable.h"

namespace irr {

CThread::CCurrentThreadHolder CThread::mCurrentHolder;


#if defined( APP_PLATFORM_WINDOWS )

#if defined(APP_DEBUG)
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
struct SThreadNameInfo {
    DWORD dwType;     // Must be 0x1000.
    LPCSTR szName;    // Pointer to name (in user addr space).
    DWORD dwThreadID; // CThread ID (-1=caller thread).
    DWORD dwFlags;    // Reserved for future use, must be zero.
};
#pragma pack(pop)

void setThreadName(DWORD dwThreadID, const char* threadName) {
    if(::IsDebuggerPresent()) {
        SThreadNameInfo info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;
        __try {
            ::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*) &info);
        } __except(EXCEPTION_CONTINUE_EXECUTION) {
        }
    }
}
#endif //APP_DEBUG


CThread::CThread() : mID(getUniqueID()),
mRunnableTarget(0),
mThread(0),
mThreadID(0),
mPriority(PRIO_NORMAL),
mPolicy(POLICY_DEFAULT),
mStackSize(APP_THREAD_STACK_SIZE) {
    mEvent.init(0, true);
    makeName();
}


CThread::CThread(const core::stringc& name) : mID(getUniqueID()),
mRunnableTarget(0),
mThread(0),
mThreadID(0),
mPriority(PRIO_NORMAL),
mPolicy(POLICY_DEFAULT),
mStackSize(APP_THREAD_STACK_SIZE),
mName(name) {

    mEvent.init(0, true);
}


CThread::~CThread() {
    if(mThread) {
        ::CloseHandle(mThread);
    }
}


void CThread::setPriority(EThreadPriority iPriority) {
    if(iPriority != mPriority) {
        mPriority = iPriority;
        if(mThread) {
            if(0 == ::SetThreadPriority(mThread, mPriority)) {
                //printf("cannot set thread priority");
            }
        }
    }
}


void CThread::start(IRunnable& target) {
    if(isRunning()) {
        return;  //printf("thread already running");
    }
    mRunnableTarget = &target;
    createThread(runnableEntry, this);
}


void CThread::start(AppCallable iTarget, void* iData) {
    if(isRunning()) {
        return; //printf("thread already running");
    }
    threadCleanup();
    mCallbackTarget.mCallback = iTarget;
    mCallbackTarget.mData = iData;
    createThread(callableEntry, this);
}


void CThread::createThread(AppThreadEntry ent, void* pData) {
#if defined(_DLL)
    mThread = ::CreateThread(NULL, mStackSize, ent, pData, 0, &mThreadID);
#else
    u32 threadId;
    mThread = (HANDLE) ::_beginthreadex(0, mStackSize, ent, this, 0, &threadId);
    mThreadID = static_cast<DWORD>(threadId);
#endif

    if(!mThread) {
        return; //printf("cannot create thread");
    }
    if(mPriority != PRIO_NORMAL && !::SetThreadPriority(mThread, mPriority)) {
        //printf("cannot set thread priority");
    }
}


void CThread::join() {
    if(!mThread) {
        return;
    }
    switch(::WaitForSingleObject(mThread, INFINITE)) {
    case WAIT_OBJECT_0:
        threadCleanup();
        return;
    default:
        //printf("cannot join thread");
        return;
    }
}


bool CThread::join(long milliseconds) {
    if(!mThread) {
        return true;
    }
    switch(::WaitForSingleObject(mThread, milliseconds + 1)) {
    case WAIT_TIMEOUT:
        return false;
    case WAIT_OBJECT_0:
        threadCleanup();
        return true;
    default:
        //printf("cannot join thread");
        return false;
    }
}


bool CThread::isRunning() const {
    if(mThread) {
        DWORD ec = 0;
        return ::GetExitCodeThread(mThread, &ec) && ec == STILL_ACTIVE;
    }
    return false;
}


void CThread::sleep(long milliseconds) {
    ::Sleep(DWORD(milliseconds));
}


void CThread::yield() {
    ::Sleep(0);
}


void CThread::threadCleanup() {
    if(!mThread) {
        return;
    }
    if(::CloseHandle(mThread)) {
        mThread = 0;
    }
}


CThread* CThread::getCurrentThread() {
    return mCurrentHolder.get();
}


CThread::TID CThread::getCurrentNativeID() {
    return ::GetCurrentThreadId();
}


s32 CThread::getMinPriority(s32 iPolicy/* = POLICY_DEFAULT*/) {
    return PRIO_LOWEST;
}


s32 CThread::getMaxPriority(s32 iPolicy/* = POLICY_DEFAULT*/) {
    return PRIO_HIGHEST;
}


#if defined(_DLL)
DWORD WINAPI CThread::runnableEntry(LPVOID iThread) {
#else
unsigned __stdcall CThread::runnableEntry(void* iThread) {
#endif
    CThread* pTI = reinterpret_cast<CThread*>(iThread);
    mCurrentHolder.set(pTI);
#if defined(APP_DEBUG)
    setThreadName(-1, pTI->getName().c_str());
#endif
    pTI->mRunnableTarget->run();
    pTI->mRunnableTarget = 0;
    return 0;
}


#if defined(_DLL)
DWORD WINAPI CThread::callableEntry(LPVOID iThread) {
#else
unsigned __stdcall CThread::callableEntry(void* iThread) {
#endif
    CThread* pTI = reinterpret_cast<CThread*>(iThread);
    mCurrentHolder.set(pTI);
#if defined(APP_DEBUG)
    setThreadName(-1, pTI->getName().c_str());
#endif
    pTI->mCallbackTarget.mCallback(pTI->mCallbackTarget.mData);
    pTI->mCallbackTarget.mCallback = 0;
    pTI->mCallbackTarget.mData = 0;
    return 0;
}


#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <time.h>


CThread::CThread() : mID(getUniqueID()),
mRunnableTarget(0),
mThreadID(0),
mPriority(PRIO_NORMAL),
mPolicy(POLICY_DEFAULT),
mStackSize(APP_THREAD_STACK_SIZE) {
    makeName();
    mEvent.init(0, true);
}

CThread::CThread(const core::stringc& name) : mID(getUniqueID()),
mRunnableTarget(0),
mThreadID(0),
mPriority(PRIO_NORMAL),
mPolicy(POLICY_DEFAULT),
mStackSize(APP_THREAD_STACK_SIZE),
mName(name) {

    mEvent.init(0, true);
}

CThread::~CThread() {
    if(isRunning()) {
        pthread_detach(mThreadID);
    }
}


bool CThread::isRunning() const {
    return mRunnableTarget || (mCallbackTarget.mCallback && mCallbackTarget.mData);
}


void CThread::yield() {
    sched_yield();
}

void CThread::setPriority(EThreadPriority iPriority) {
    if(iPriority != mPriority) {
        mPriority = iPriority;
        if(isRunning()) {
            struct sched_param par;
            par.sched_priority = mapPrio(mPriority, SCHED_OTHER);
            if(pthread_setschedparam(mThreadID, SCHED_OTHER, &par)) {
                //printf("cannot set thread priority");
            }
        }
    }
}


s32 CThread::getMinPriority(s32 iPolicy/* = POLICY_DEFAULT*/) {
    return sched_get_priority_min(iPolicy);
}


s32 CThread::getMaxPriority(s32 iPolicy/* = POLICY_DEFAULT*/) {
    return sched_get_priority_max(iPolicy);
}


void CThread::start(IRunnable& target) {
    if(mRunnableTarget) {
        //printf("thread already running");
        return;
    }
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    if(mStackSize != 0) {
        if(0 != pthread_attr_setstacksize(&attributes, mStackSize)) {
            pthread_attr_destroy(&attributes);
            //printf("cannot set thread stack size");
        }
    }

    mRunnableTarget = &target;
    if(pthread_create(&mThreadID, &attributes, runnableEntry, this)) {
        mRunnableTarget = 0;
        pthread_attr_destroy(&attributes);
        //printf("cannot start thread");
    }
    pthread_attr_destroy(&attributes);

    if(mPolicy == SCHED_OTHER) {
        if(mPriority != PRIO_NORMAL) {
            struct sched_param par;
            par.sched_priority = mapPrio(mPriority, SCHED_OTHER);
            if(pthread_setschedparam(mThreadID, SCHED_OTHER, &par)) {
                //printf("cannot set thread priority");
            }
        }
    } else {
        struct sched_param par;
        par.sched_priority = mapPrio(mPriority, mPolicy);
        if(pthread_setschedparam(mThreadID, mPolicy, &par)) {
            //printf("cannot set thread priority");
        }
    }
}


void CThread::start(AppCallable target, void* pData) {
    if(mCallbackTarget.mCallback) {
        //printf("thread already running");
        return;
    }
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    if(mStackSize != 0) {
        if(0 != pthread_attr_setstacksize(&attributes, mStackSize)) {
            //printf("can not set thread stack size");
        }
    }

    mCallbackTarget.mCallback = target;
    mCallbackTarget.mData = pData;

    if(pthread_create(&mThreadID, &attributes, callableEntry, this)) {
        mCallbackTarget.mCallback = 0;
        mCallbackTarget.mData = 0;
        //printf("cannot start thread");
    }

    if(mPolicy == SCHED_OTHER) {
        if(mPriority != PRIO_NORMAL) {
            struct sched_param par;
            par.sched_priority = mapPrio(mPriority, SCHED_OTHER);
            if(pthread_setschedparam(mThreadID, SCHED_OTHER, &par)) {
                //printf("cannot set thread priority");
            }
        }
    } else {
        struct sched_param par;
        par.sched_priority = mPriority;
        if(pthread_setschedparam(mThreadID, mPolicy, &par)) {
            //printf("cannot set thread priority");
        }
    }
}


void CThread::join() {
    //mEvent.wait();
    void* result;
    if(pthread_join(mThreadID, &result)) {
        //printf("cannot join thread"); 
    }
}


bool CThread::join(long milliseconds) {
    if(mEvent.wait(milliseconds)) {
        void* result;
        if(pthread_join(mThreadID, &result)) {
            //printf("cannot join thread");
        }
        return true;
    } else {
        return false;
    }
}


CThread* CThread::getCurrentThread() {
    return mCurrentHolder.get();
}



CThread::TID CThread::getCurrentNativeID() {
    return pthread_self();
}


void CThread::sleep(long milliseconds) {
    struct timespec ts;
    struct timespec leftover;
    s32 rc;
    ts.tv_sec = (long) milliseconds / 1000;
    ts.tv_nsec = (long) milliseconds % 1000 * 1000000;

    do {
        rc = ::nanosleep(&ts, &leftover);
        if(rc < 0) {
            if(errno == EINTR) {
                ts = leftover;
            } else {
                //printf("CThread::sleep(): nanosleep() failed");
                break;
            }
        }
    } while(rc != 0);

#if 0
    * Timespan remainingTime(1000 * *Timespan::TimeDiff(milliseconds));
    s32 rc;
    do {
        struct timeval tv;
        tv.tv_sec = (long) remainingTime.totalSeconds();
        tv.tv_usec = (long) remainingTime.useconds();
        *Timestamp start;
        rc = ::select(0, NULL, NULL, NULL, &tv);
        if(rc < 0 && errno == EINTR) {
            *Timestamp end;
            *Timespan waited = start.elapsed();
            if(waited < remainingTime)
                remainingTime -= waited;
            else
                remainingTime = 0;
        }
    } while(remainingTime > 0 && rc < 0 && errno == EINTR);
    if(rc < 0 && remainingTime > 0) {
        //printf("CThread::sleep(): select() failed");
    }
#endif
}


void* CThread::runnableEntry(void* iThread) {
    CThread* pThreadImpl = reinterpret_cast<CThread*>(iThread);
    mCurrentHolder.set(pThreadImpl);

    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGQUIT);
    sigaddset(&sset, SIGTERM);
    sigaddset(&sset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sset, 0);

    pThreadImpl->mRunnableTarget->run();
    pThreadImpl->mRunnableTarget = 0;
    //pThreadImpl->mEvent.set();
    return 0;
}


void* CThread::callableEntry(void* iThread) {
    CThread* pThreadImpl = reinterpret_cast<CThread*>(iThread);
    mCurrentHolder.set(pThreadImpl);

    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGQUIT);
    sigaddset(&sset, SIGTERM);
    sigaddset(&sset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sset, 0);

    pThreadImpl->mCallbackTarget.mCallback(pThreadImpl->mCallbackTarget.mData);
    pThreadImpl->mCallbackTarget.mCallback = 0;
    pThreadImpl->mCallbackTarget.mData = 0;
    //pThreadImpl->mEvent.set();
    return 0;
}


s32 CThread::mapPrio(s32 iPriority, s32 iPolicy) {
    s32 pmin = getMinPriority(iPolicy);
    s32 pmax = getMaxPriority(iPolicy);

    switch(iPriority) {
    case PRIO_LOWEST:
        return pmin;
    case PRIO_LOW:
        return pmin + (pmax - pmin) / 4;
    case PRIO_NORMAL:
        return pmin + (pmax - pmin) / 2;
    case PRIO_HIGH:
        return pmin + 3 * (pmax - pmin) / 4;
    case PRIO_HIGHEST:
        return pmax;
    default:
        //("invalid thread priority");
        return -1; // just to satisfy compiler - we'll never get here anyway
    }
}

#endif //posix thread of APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID



bool CThread::wait(long milliseconds) {
    CThread* thr = CThread::getCurrentThread();
    return !(thr->mEvent.wait(milliseconds));
}


bool wait(CThreadEvent& iEvent, long milliseconds) {
    CThread* thr = CThread::getCurrentThread();
    return !(iEvent.wait(milliseconds));
}


void CThread::wakeUp() {
    mEvent.set();
}


void CThread::makeName() {
    mName = "#";
    mName.append(core::stringc(mID));
}


s32 CThread::getUniqueID() {
    static CMutex fastMutex(EMT_NO_RECURSIVE);
    static u32 count = 0;
    CAutoLock lock(fastMutex);
    ++count;
    return count;
}


void CThread::setName(const core::stringc& name) {
    static CMutex fastMutex(EMT_NO_RECURSIVE);
    CAutoLock lock(fastMutex);
    mName = name;
}


} //end namespace irr