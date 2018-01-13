#include "CNamedMutex.h"

#if defined( APP_PLATFORM_WINDOWS )
#include <Windows.h>
#endif

#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <pthread.h>
#endif


#if defined( APP_PLATFORM_WINDOWS )

namespace irr {
CNamedMutex::CNamedMutex(const io::path& iName, EMutexType type/* = EMT_RECURSIVE*/) : mMutex(0),
mName(iName) {
    mMutex = ::CreateMutex(0, FALSE, mName.c_str());
}


CNamedMutex::~CNamedMutex() {
    ::CloseHandle(mMutex);
}


void CNamedMutex::lock() {
    switch(::WaitForSingleObject(mMutex, INFINITE)) {
    case WAIT_OBJECT_0:
        return;
    case WAIT_ABANDONED:
        //("cannot lock named mutex (abadoned)", mName);
        break;
    default:
        //("cannot lock named mutex", mName);
        break;
    }
}


bool CNamedMutex::tryLock() {
    switch(::WaitForSingleObject(mMutex, 0)) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        break;
    case WAIT_ABANDONED:
        //("cannot lock named mutex (abadoned)", mName);
        break;
    default:
        //("cannot lock named mutex", mName);
        break;
    }
    return false;
}


void CNamedMutex::unlock() {
    ::ReleaseMutex(mMutex);
}

}//irr
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#if defined(APP_HAVE_MUTEX_TIMEOUT)
#include <sys/time.h>
#endif //APP_HAVE_MUTEX_TIMEOUT

namespace irr {

CNamedMutex::CNamedMutex(const io::path& iName, EMutexType type/* = EMT_RECURSIVE*/) :mName(iName) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
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


CNamedMutex::~CNamedMutex() {
    pthread_mutex_destroy(&mMutex);
}


void CNamedMutex::lock() {
    if(pthread_mutex_lock(&mMutex)) {
        //("cannot lock mutex");
    }
}


bool CNamedMutex::tryLock() {
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


void CNamedMutex::unlock() {
    if(pthread_mutex_unlock(&mMutex)) {
        //("cannot unlock mutex");
    }
}


} //namespace irr
#endif //APP_PLATFORM_LINUX
