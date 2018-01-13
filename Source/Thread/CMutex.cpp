#include "CMutex.h"
#include "irrTypes.h"


#if defined( APP_PLATFORM_WINDOWS )
namespace irr {

CMutex::CMutex(EMutexType type/* = EMT_RECURSIVE*/) {
    ::InitializeCriticalSectionAndSpinCount(&mCriticalSection, 4000);
}

CMutex::~CMutex() {
    ::DeleteCriticalSection(&mCriticalSection);
}

bool CMutex::tryLock(long milliseconds) {
    const s32 sleepMillis = 5;
    u32 iTime = 0;
    do {
        if(::TryEnterCriticalSection(&mCriticalSection) == TRUE) {
            return true;
        }
        ::Sleep(sleepMillis);
        iTime += sleepMillis;
    } while(iTime <= milliseconds);

    return false;
}

void CMutex::lock() {
    ::EnterCriticalSection(&mCriticalSection);
}

bool CMutex::tryLock() {
    return TRUE == ::TryEnterCriticalSection(&mCriticalSection);
}

void CMutex::unlock() {
    ::LeaveCriticalSection(&mCriticalSection);
}

void* CMutex::getHandle() {
    return &mCriticalSection;
}

} //namespace irr

#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )

#if defined(APP_HAVE_MUTEX_TIMEOUT)
#include <sys/time.h>
#endif //APP_HAVE_MUTEX_TIMEOUT

namespace irr {

CMutex::CMutex(EMutexType type/* = EMT_RECURSIVE*/) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
    switch(type) {
    default:
    case EMT_RECURSIVE:
        //pthread_mutexattr_settype_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        break;
    case EMT_NO_RECURSIVE:
        //pthread_mutexattr_settype_np(&attr, PTHREAD_MUTEX_NORMAL_NP);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        break;
    }
    if(pthread_mutex_init(&mMutex, &attr)) {
        //("cannot create mutex");
    }
    pthread_mutexattr_destroy(&attr);
}


CMutex::~CMutex() {
    pthread_mutex_destroy(&mMutex);
}

void* CMutex::getHandle() {
    return &mMutex;
}


bool CMutex::tryLock(long milliseconds) {
#if defined(APP_HAVE_MUTEX_TIMEOUT)
    struct timespec abstime;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    abstime.tv_sec = tv.tv_sec + milliseconds / 1000;
    abstime.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
    if(abstime.tv_nsec >= 1000000000) {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }
    s32 rc = pthread_mutex_timedlock(&mMutex, &abstime);
    if(rc == 0) {
        return true;
    } else if(rc == ETIMEDOUT) {
        return false;
    } else {
        //("cannot lock mutex");
        return false;
    }
#else
    const s32 sleepMillis = 5;
    u32 iTime = 0;
    do {
        s32 rc = pthread_mutex_trylock(&mMutex);
        if(rc == 0) {
            return true;
        } else if(rc != EBUSY) {
            //throw ("cannot lock mutex");
            break;
        }

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = sleepMillis * 1000000;
        nanosleep(&ts, NULL);
        iTime += sleepMillis;
        /*
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = sleepMillis * 1000;
        select(0, NULL, NULL, NULL, &tv);
        */
    } while(iTime <= milliseconds);

    return false;
#endif
}


void CMutex::lock() {
    if(pthread_mutex_lock(&mMutex)) {
        //("cannot lock mutex");
    }
}


bool CMutex::tryLock() {
    switch(pthread_mutex_trylock(&mMutex)) {
    case 0:
        return true;
    case EBUSY: //The mutex could not be acquired because it was already locked.
        return false;
    case EINVAL: //mutex have not been initialized.
    case EAGAIN: //exceed max lock count.
    default:
        return false;
    }
}


void CMutex::unlock() {
    if(pthread_mutex_unlock(&mMutex)) {
        //("cannot unlock mutex");
    }
}


} //namespace irr
#endif
