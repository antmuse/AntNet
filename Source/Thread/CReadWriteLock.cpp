#include "CReadWriteLock.h"

#if defined( APP_PLATFORM_WINDOWS )

namespace irr {

CReadWriteLock::CReadWriteLock() {
    ::InitializeSRWLock(&mLocker);
}


CReadWriteLock::~CReadWriteLock() {
}


bool CReadWriteLock::tryLockWrite() {
    return 0 != ::TryAcquireSRWLockExclusive(&mLocker);
}

void CReadWriteLock::lockWrite() {
    ::AcquireSRWLockExclusive(&mLocker);
}

void CReadWriteLock::unlockWrite() {
    ::ReleaseSRWLockExclusive(&mLocker);
}


bool CReadWriteLock::tryLockRead() {
    return 0 != ::TryAcquireSRWLockShared(&mLocker);
}


void CReadWriteLock::lockRead() {
    ::AcquireSRWLockShared(&mLocker);
}


void CReadWriteLock::unlockRead() {
    ::ReleaseSRWLockShared(&mLocker);
}
} //namespace irr

#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )

namespace irr {

CReadWriteLock::CReadWriteLock() {
    if(pthread_rwlock_init(&mLocker, NULL)) {
        //("cannot create reader/writer lock");
    }
}


CReadWriteLock::~CReadWriteLock() {
    pthread_rwlock_destroy(&mLocker);
}


bool CReadWriteLock::tryLockRead() {
    return 0 == pthread_rwlock_tryrdlock(&mLocker);
}


void CReadWriteLock::lockRead() {
    if(pthread_rwlock_rdlock(&mLocker)) {
        //("cannot lock reader/writer lock");
    }
}


void CReadWriteLock::unlockRead() {
    if(pthread_rwlock_unlock(&mLocker)) {
        //("cannot unlock mutex");
    }
}


bool CReadWriteLock::tryLockWrite() {
    return 0 == pthread_rwlock_trywrlock(&mLocker);
}


void CReadWriteLock::lockWrite() {
    if(pthread_rwlock_wrlock(&mLocker)) {
        //("cannot lock reader/writer lock");
    }
}


void CReadWriteLock::unlockWrite() {
    if(pthread_rwlock_unlock(&mLocker)) {
        //("cannot unlock mutex");
    }
}
} //namespace irr


#endif //( APP_PLATFORM_ANDROID )  || ( APP_PLATFORM_LINUX )
