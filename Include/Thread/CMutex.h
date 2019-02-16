/**
*@file CMutex.h
*@brief This file defined a mutex (MUTual EXclusion) object,
* that allows a thread to lock critical instructions
* to avoid simultaneous access with other threads.
*@author antmuse@live.cn
*@date 2014-09-22
*/


#ifndef APP_CMUTEX_H
#define APP_CMUTEX_H

#include "HConfig.h"
#include "HMutexType.h"

#if defined( APP_PLATFORM_WINDOWS )
#include <winsock2.h>   //just here to prevent <winsock.h>
#include <Windows.h>
#endif
#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#endif

namespace irr {

/**
*@class CMutex
*@brief A CMutex (mutual exclusion) is a synchronization
* mechanism used to control access to a shared resource
* in a concurrent (multithreaded) scenario.
* Using the CAutoLock class is the preferred way to automatically
* lock and unlock a mutex.
*/
class CMutex {
public:

    /**
    *@param type Mutex type.
    */
    CMutex(EMutexType type = EMT_RECURSIVE);

    ~CMutex();

    /**
    *@brief  Locks the mutex. Blocks if the mutex
    *is held by another thread.
    */
    void lock();


    /**
    *@brief Locks the mutex. Blocks up to the given number of milliseconds
    * if the mutex is held by another thread.
    *@param milliseconds Block time in millisecond.
    *@return false if the mutex can not be locked within the given timeout, else true.
    */
    bool lock(long milliseconds) {
        return tryLock(milliseconds);
    }


    /**
    *@brief Tries to lock the mutex. Returns false immediately
    * if the mutex is already held by another thread.
    *@param milliseconds Block time in millisecond.
    *@return true if the mutex was successfully locked, else false.
    */
    bool tryLock();


    /**
    *@brief Locks the mutex. Blocks up to the given number of milliseconds
    * if the mutex is held by another thread.
    *@param milliseconds Block time in millisecond.
    *@return true if the mutex was successfully locked, else false.
    */
    bool tryLock(long milliseconds);


    /// Unlocks the mutex so that it can be acquired by	other threads.
    void unlock();

    void* getHandle();


private:
    CMutex(const CMutex& it) = delete;
    CMutex& operator=(const CMutex& it) = delete;

#if defined( APP_PLATFORM_WINDOWS )
    CRITICAL_SECTION mCriticalSection;
#endif

#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    pthread_mutex_t mMutex;
#endif
};



/**
*@class CNullMutex
*@brief A CNullMutex is an simple mutex implementation
* which performs no locking at all. Useful in policy driven design
* where the type of mutex used can be now a template parameter allowing
* to switch between thread-safe and not thread-safe implementations.
*/
class CNullMutex {
public:
    CNullMutex() {
    }

    ~CNullMutex() {
    }

    void lock() {
    }

    void lock(long) {
    }

    bool tryLock() {
        return true;
    }

    bool tryLock(long) {
        return true;
    }

    void unlock() {
    }
};


/**
*@class CAutoLock
*/
class CAutoLock {
public:

    CAutoLock(CMutex& iMut) : mMutex(iMut) {
        mMutex.lock();
    }

    ~CAutoLock() {
        mMutex.unlock();
    }
private:
    //CAutoLock() {
    //}

    CMutex& mMutex;
};


/**
*@class CAutoReverseLock
*/
class CAutoReverseLock {
public:
    CAutoReverseLock(CMutex& it) : mMutex(it) {
        mMutex.unlock();
    }

    ~CAutoReverseLock() {
        mMutex.lock();
    }

private:
    //CAutoReverseLock(){
    //}

    CMutex& mMutex;
};


} //end namespace irr

#endif	/* APP_CMUTEX_H */
