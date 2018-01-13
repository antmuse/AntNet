#include "CThreadEvent.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
//
#endif

namespace irr {


#if defined(APP_PLATFORM_WINDOWS)
CThreadEvent::CThreadEvent() : mHandle(0) {
}


bool CThreadEvent::open(const fschar_t* iName, bool inherit) {
    if(0 == mHandle) {
        mHandle = ::OpenEvent(EVENT_ALL_ACCESS, inherit, iName);
    }
    return 0 != mHandle;
}


bool CThreadEvent::init(const fschar_t* iName, bool autoReset) {
    if(0 == mHandle) {
        mHandle = ::CreateEvent(NULL, autoReset ? FALSE : TRUE, FALSE, iName);
    }
    return 0 != mHandle;
}


CThreadEvent::~CThreadEvent() {
    ::CloseHandle(mHandle);
}


void CThreadEvent::set() {
    if(0 == ::SetEvent(mHandle)) {
        //printf("cannot signal event");
    }
}


void  CThreadEvent::reset() {
    if(0 == ::ResetEvent(mHandle)) {
        //printf("cannot reset event");
    }
}


bool CThreadEvent::wait() {
    switch(::WaitForSingleObject(mHandle, INFINITE)) {
    case WAIT_OBJECT_0:
        return true;
    default:
        break;//printf("wait for event failed");
    }
    return false;
}


bool CThreadEvent::wait(long milliseconds) {
    switch(::WaitForSingleObject(mHandle, milliseconds + 1)) {
    case WAIT_TIMEOUT:
        return false;
    case WAIT_OBJECT_0:
        return true;
    default:
        break; //printf("wait for event failed");
    }
    return false;
}




#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
CThreadEvent::CThreadEvent() : mStatus(false), mAutoReset(true) {
}


bool CThreadEvent::open(const fschar_t* iName, bool inherit) {
    //TODO
    return true;
}


bool CThreadEvent::init(const fschar_t* iName, bool autoReset) {
    mAutoReset = autoReset;
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    if(pthread_mutex_init(&mMutex, &mutexattr)) {
        pthread_mutexattr_destroy(&mutexattr);
        return false;// printf("cannot create event (mutex)");
    }

    pthread_mutexattr_destroy(&mutexattr);

    pthread_condattr_t attr;
    if(pthread_condattr_init(&attr)) {
        pthread_mutex_destroy(&mMutex);
        return false;// printf("cannot create event (condition attribute)");
    }
    if(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        pthread_condattr_destroy(&attr);
        pthread_mutex_destroy(&mMutex);
        return false;// printf("cannot create event (condition attribute clock)");
    }
    if(pthread_cond_init(&mCond, &attr)) {
        pthread_condattr_destroy(&attr);
        pthread_mutex_destroy(&mMutex);
        return false;// printf("cannot create event (condition)");
    }
    return true;
}


CThreadEvent::~CThreadEvent() {
    pthread_cond_destroy(&mCond);
    pthread_mutex_destroy(&mMutex);
}


void CThreadEvent::set() {
    if(::pthread_mutex_lock(&mMutex)) {
        return;
    }
    mStatus = true;
    if(::pthread_cond_broadcast(&mCond)) {
        return;
    }
    ::pthread_mutex_unlock(&mMutex);
}


void  CThreadEvent::reset() {
    if(::pthread_mutex_lock(&mMutex)) {
        return;
    }
    mStatus = false;
    ::pthread_mutex_unlock(&mMutex);
}


bool CThreadEvent::wait() {
    if(::pthread_mutex_lock(&mMutex)) {
        return false;
    }
    while(!mStatus) {
        if(::pthread_cond_wait(&mCond, &mMutex)) {
            ::pthread_mutex_unlock(&mMutex);
        }
    }
    if(mAutoReset) {
        mStatus = false;
    }
    ::pthread_mutex_unlock(&mMutex);
}


bool CThreadEvent::wait(long milliseconds) {
    s32 rc = 0;
    struct timespec abstime;
    ::clock_gettime(CLOCK_MONOTONIC, &abstime);
    abstime.tv_sec += milliseconds / 1000;
    abstime.tv_nsec += (milliseconds % 1000) * 1000000;
    if(abstime.tv_nsec >= 1000000000) {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }

#if 0
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    abstime.tv_sec = tv.tv_sec + milliseconds / 1000;
    abstime.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
    if(abstime.tv_nsec >= 1000000000) {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }
#endif


    if(::pthread_mutex_lock(&mMutex) != 0) {
        // ("wait for event failed (lock)");
    }
    while(!mStatus) {
        if((rc = ::pthread_cond_timedwait(&mCond, &mMutex, &abstime))) {
            if(rc == ETIMEDOUT) break;
            ::pthread_mutex_unlock(&mMutex);
            // printf("cannot wait for event");
        }
    }
    if(rc == 0 && mAutoReset) {
        mStatus = false;
    }
    ::pthread_mutex_unlock(&mMutex);

    return rc == 0;
}

#endif //APP_PLATFORM_LINUX


} //end namespace irr
