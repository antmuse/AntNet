/**
*@file CReadWriteLock.h
*@brief This file defined a read/write locker.
*@author antmuse@live.cn
*@date 2014-11-12
*/

#ifndef APP_CREADWRITELOCK_H
#define APP_CREADWRITELOCK_H

#include "HConfig.h"
#include "irrTypes.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>   //just here to prevent <windock.h>
#include <Windows.h>
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <pthread.h>
#include <errno.h>
#endif


namespace irr {

    /**
    *@class CReadWriteLock
    *@brief A reader writer lock allows multiple concurrent readers or one exclusive writer.
    */
    class CReadWriteLock {
    public:
        CReadWriteLock();

        ~CReadWriteLock();

        /**
        *@brief Acquires a read lock. If another thread currently holds a write lock,waits until the write lock is released.
        */
        void lockRead();


        /**
        *@brief Acquires a write lock. If one or more other threads currently hold locks, 
        * waits until all locks are released. 
        * The results are undefined if the same thread already holds a read or write lock.
        */
        void lockWrite();

        /**
        *@brief Tries to acquire a read lock.
        *@return Immediately returns true if successful, or false if another thread currently holds a write lock.
        */
        bool tryLockRead();

        /**
        *@brief Tries to acquire a write lock.
        *@return Immediately returns true if successful,or false if one or more other threads currently hold locks.
        *The result is undefined if the same thread already holds a read or write lock.
        */
        bool tryLockWrite();


        /// Release the read lock.
        void unlockRead();

        /// Release the write lock.
        void unlockWrite();


    private:
        CReadWriteLock(const CReadWriteLock&);

        CReadWriteLock& operator = (const CReadWriteLock&);

#if defined(APP_PLATFORM_WINDOWS)
        SRWLOCK mLocker;
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
        pthread_rwlock_t mLocker;
#endif //APP_PLATFORM_WINDOWS
    };


} // namespace irr


#endif // APP_CREADWRITELOCK_H
