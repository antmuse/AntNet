/**
*@file CNamedMutex.h
*@brief This file defined named mutex(can used for processes)
*@author antmuse@live.cn
*@date 2014-11-12
*/

#ifndef APP_CNAMEDMUTEX_H
#define APP_CNAMEDMUTEX_H

#include "HConfig.h"
#include "path.h"
#include "HMutexType.h"
#if defined(APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#endif


namespace irr {
/**
*@class  CNamedMutex
*@brief A CNamedMutex(mutual exclusion) is a global synchronization
* mechanism used to control access to a shared resource
* in a concurrent (multi process) scenario.
* Using the CAutoNamedLock class is the preferred way to automatically
* lock and unlock a mutex.
*
* A CNamedMutex refers to a named operating system resource being the
* unit of synchronization.
* In other words, there can be multiple instances of CNamedMutex referring
* to the same actual synchronization object.
*
* There should not be more than one instance of CNamedMutex for
* a given name in a process. Otherwise, the instances may
* interfere with each other.
*/
class  CNamedMutex {
public:

    /// Constructor
    CNamedMutex(const io::path& iName, EMutexType type = EMT_RECURSIVE);

    /// Destructor
    ~CNamedMutex();

    /// Locks the mutex. Blocks if the mutex is held by another process or thread.
    void lock();

    /**
    *@brief Tries to lock the mutex.
    *@return false immediately if the mutex is already held by another process or thread,
    * else true if the mutex was successfully locked.
    */
    bool tryLock();

    /// Unlock the mutex so that it can be acquired by other threads.
    void unlock();

private:
    CNamedMutex();
    CNamedMutex(const CNamedMutex&);
    CNamedMutex& operator = (const CNamedMutex&);

    io::path mName;

#if defined( APP_PLATFORM_WINDOWS )
    void* mMutex; //HANDLE
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    pthread_mutex_t mMutex;
#endif
};


}//irr


#endif	/* APP_CNAMEDMUTEX_H */