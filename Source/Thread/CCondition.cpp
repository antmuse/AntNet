#include "CCondition.h"
#include "CMutex.h"
#include "CReadWriteLock.h"

namespace irr {


#if defined(APP_PLATFORM_WINDOWS)
CCondition::CCondition() {
    //APP_ASSERT(sizeof(mCondition) == sizeof(CONDITION_VARIABLE));
    ::InitializeConditionVariable(&mCondition);
}


CCondition::~CCondition() {
}


bool CCondition::notify() {
    ::WakeConditionVariable(&mCondition);
    return true;
}


bool CCondition::notifyAll() {
    ::WakeAllConditionVariable(&mCondition);
    return true;
}

bool CCondition::wait(CMutex& mutex) {
    return (TRUE == ::SleepConditionVariableCS(&mCondition, (CRITICAL_SECTION*) mutex.getHandle(), INFINITE));
}


bool CCondition::wait(CMutex& mutex, u32 milliseconds) {
    return (TRUE == ::SleepConditionVariableCS(&mCondition, (CRITICAL_SECTION*) mutex.getHandle(), milliseconds));
}

bool CCondition::waitWrite(CReadWriteLock& mutex) {
    return (TRUE == ::SleepConditionVariableSRW(&mCondition,
        (SRWLOCK*) mutex.getHandle(), INFINITE, 0));
}

bool CCondition::waitRead(CReadWriteLock& mutex) {
    return (TRUE == ::SleepConditionVariableSRW(&mCondition,
        (SRWLOCK*) mutex.getHandle(), INFINITE,
        CONDITION_VARIABLE_LOCKMODE_SHARED));
}

bool CCondition::waitWrite(CReadWriteLock& mutex, u32 milliseconds) {
    return (TRUE == ::SleepConditionVariableSRW(&mCondition,
        (SRWLOCK*) mutex.getHandle(), milliseconds, 0));
}

bool CCondition::waitRead(CReadWriteLock& mutex, u32 milliseconds) {
    return (TRUE == ::SleepConditionVariableSRW(&mCondition,
        (SRWLOCK*) mutex.getHandle(), milliseconds,
        CONDITION_VARIABLE_LOCKMODE_SHARED));
}


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

CCondition::CCondition() {
    init();
}


bool CCondition::init() {
    pthread_condattr_t attr;
    if(::pthread_condattr_init(&attr)) {
        return false;
    }
    if(::pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        ::pthread_condattr_destroy(&attr);
        return false;
    }
    if(::pthread_cond_init(&mCondition, &attr)) {
        ::pthread_condattr_destroy(&attr);
        return false;
    }
    return true;
}


CCondition::~CCondition() {
    ::pthread_cond_destroy(&mCondition);
}


bool CCondition::notify() {
    return (0 == ::pthread_cond_signal(&mCondition));
}

bool CCondition::notifyAll() {
    return (0 == ::pthread_cond_broadcast(&mCondition));
}


bool CCondition::wait(CMutex& mutex) {
    return (0 == ::pthread_cond_wait(&mCondition, (pthread_mutex_t*) mutex.getHandle()));
}


bool CCondition::wait(CMutex& mutex, u32 milliseconds) {
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
    return 0 == ::pthread_cond_timedwait(&mCondition, (pthread_mutex_t*) mutex.getHandle(), &abstime);
}

#endif //APP_PLATFORM_LINUX
} //end namespace irr
