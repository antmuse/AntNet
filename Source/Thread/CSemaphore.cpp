#include "CSemaphore.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <time.h>
#include <sys/time.h>
#endif



#if defined( APP_PLATFORM_WINDOWS )

namespace irr {
CSemaphore::CSemaphore() : mSema(0) {
}


bool CSemaphore::open(fschar_t* iName, bool inherit) {
    APP_ASSERT(iName);
    if(0 == mSema) {
        mSema = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, inherit ? TRUE : FALSE, iName);
    }
    return 0 != mSema;
}


bool CSemaphore::init(fschar_t* iName, s32 n, s32 max) {
    APP_ASSERT(n >= 0 && max > 0 && n <= max);
    if(0 == mSema) {
        mSema = ::CreateSemaphore(0, n, max, iName);
    }
    return 0 != mSema;
}


CSemaphore::~CSemaphore() {
    ::CloseHandle(mSema);
}


void CSemaphore::set() {
    if(!::ReleaseSemaphore(mSema, 1, NULL)) {
        //("cannot signal semaphore");
    }
}

void CSemaphore::wait() {
    switch(::WaitForSingleObject(mSema, INFINITE)) {
    case WAIT_OBJECT_0:
        return;
    default:
        break;//("wait for semaphore failed");
    }
}


bool CSemaphore::wait(long milliseconds) {
    switch(::WaitForSingleObject(mSema, milliseconds + 1)) {
    case WAIT_TIMEOUT:
        return false;
    case WAIT_OBJECT_0:
        return true;
    default:
        return false; //("wait for semaphore failed");
    }
}


} //namespace irr

#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )


//
// Note: pthread_cond_timedwait() with CLOCK_MONOTONIC is supported
// on Linux and QNX, as well as on Android >= 5.0. On Android < 5.0,
// HAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC is defined to indicate
// availability of non-standard pthread_cond_timedwait_monotonic().
//
#ifndef APP_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT
#if (defined(__linux__) || defined(__QNX__)) && !(defined(__ANDROID__) && defined(HAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC))
#define APP_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT 1
#endif
#endif


#ifndef APP_HAVE_CLOCK_GETTIME
#if (defined(_POSIX_TIMERS) && defined(CLOCK_REALTIME)) || defined(POCO_VXWORKS) || defined(__QNX__)
#ifndef __APPLE__ // See GitHub issue #1453 - not available before Mac OS 10.12/iOS 10
#define APP_HAVE_CLOCK_GETTIME
#endif
#endif
#endif


namespace irr {
CSemaphore::CSemaphore() : mValue(0), mMax(1) {
}

bool CSemaphore::open(fschar_t* iName, bool inherit) {
    //TODO>>
    //mValue = n;
    //mMax = ?
    return true;
}


bool CSemaphore::init(fschar_t* iName, s32 n, s32 max) {
    APP_ASSERT(n >= 0 && max > 0 && n <= max);
    mValue = n;
    mMax = max;
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, iName ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE);

    if(pthread_mutex_init(&mMutex, &mutexattr)) {
        pthread_mutexattr_destroy(&mutexattr);
        return false;//("cannot create semaphore (mutex)");
    }

    pthread_mutexattr_destroy(&mutexattr);

#if defined(APP_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT)
    pthread_condattr_t attr;
    if(pthread_condattr_init(&attr)) {
        pthread_mutex_destroy(&mMutex);
        return false;//("cannot create semaphore (condition attribute)");
    }
    if(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        pthread_condattr_destroy(&attr);
        pthread_mutex_destroy(&mMutex);
        return false;//("cannot create semaphore (condition attribute clock)");
    }
    if(pthread_cond_init(&mCond, &attr)) {
        pthread_condattr_destroy(&attr);
        pthread_mutex_destroy(&mMutex);
        return false;//("cannot create semaphore (condition)");
    }
    pthread_condattr_destroy(&attr);
#else
    if(pthread_cond_init(&mCond, NULL)) {
        pthread_mutex_destroy(&mMutex);
        return false;//("cannot create semaphore (condition)");
    }
#endif
    return true;
}


CSemaphore::~CSemaphore() {
    pthread_cond_destroy(&mCond);
    pthread_mutex_destroy(&mMutex);
}


void CSemaphore::wait() {
    if(pthread_mutex_lock(&mMutex)) {
        return; //("wait for semaphore failed (lock)");
    }
    s32 ret;
    while(mValue < 1) {
        if(ret = pthread_cond_wait(&mCond, &mMutex)) {
            pthread_mutex_unlock(&mMutex);
            break;//("wait for semaphore failed");
        }
    }
    if(0 == ret) {
        --mValue;
    }
    pthread_mutex_unlock(&mMutex);
}


bool CSemaphore::wait(long milliseconds) {
    s32 rc = 0;
    struct timespec abstime;

#if defined(__VMS)
    struct timespec delta;
    delta.tv_sec = milliseconds / 1000;
    delta.tv_nsec = (milliseconds % 1000) * 1000000;
    pthread_get_expiration_np(&delta, &abstime);
#elif defined(APP_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT)
    clock_gettime(CLOCK_MONOTONIC, &abstime);
    abstime.tv_sec += milliseconds / 1000;
    abstime.tv_nsec += (milliseconds % 1000) * 1000000;
    if(abstime.tv_nsec >= 1000000000) {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }
#elif defined(APP_HAVE_CLOCK_GETTIME)
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += milliseconds / 1000;
    abstime.tv_nsec += (milliseconds % 1000) * 1000000;
    if(abstime.tv_nsec >= 1000000000) {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    abstime.tv_sec = tv.tv_sec + milliseconds / 1000;
    abstime.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
    if(abstime.tv_nsec >= 1000000000) {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }
#endif

    if(pthread_mutex_lock(&mMutex) != 0)
        return false; //("wait for semaphore failed (lock)");

    while(mValue < 1) {
        if((rc = pthread_cond_timedwait(&mCond, &mMutex, &abstime))) {

            if(rc == ETIMEDOUT) break;

            pthread_mutex_unlock(&mMutex);
            return false; //("cannot wait for semaphore");
        }
    }

    if(rc == 0) --mValue;

    pthread_mutex_unlock(&mMutex);
    return rc == 0;
}

void CSemaphore::set() {
    if(pthread_mutex_lock(&mMutex))
        return; //("cannot signal semaphore (lock)");

    if(mValue < mMax) {
        ++mValue;
    } else {
        pthread_mutex_unlock(&mMutex);
        return; //("cannot signal semaphore: count would exceed maximum");
    }
    if(pthread_cond_signal(&mCond)) {
        pthread_mutex_unlock(&mMutex);
        return; //("cannot signal semaphore");
    }
    pthread_mutex_unlock(&mMutex);
}


} //namespace irr


#endif //( APP_PLATFORM_ANDROID )  || ( APP_PLATFORM_LINUX )
