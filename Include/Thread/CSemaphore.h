/**
*@file CSemaphore.h
*@brief This file defined a semaphore.
*@author antmuse@live.cn
*@date 2014-11-12
*/

#ifndef APP_CREADWRITELOCK_H
#define APP_CREADWRITELOCK_H

#include "HConfig.h"
#include "irrTypes.h"
#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <pthread.h>
#include <errno.h>
#endif


namespace irr {

    /**
    *@class CSemaphore
    *@brief A CSemaphore is a synchronization object with the following 
    * characteristics:
    * A semaphore has a value that is constrained to be a non-negative
    * integer and two atomic operations. The allowable operations are V 
    * (here called set()) and P (here called wait()). A V (set()) operation 
    * increases the value of the semaphore by one. 
    * A P (wait()) operation decreases the value of the semaphore by one, 
    * provided that can be done without violating the constraint that the 
    * value be non-negative. A P (wait()) operation that is initiated when 
    * the value of the semaphore is 0 suspends the calling thread. 
    * The calling thread may continue when the value becomes positive again.
    */
    class CSemaphore{
    public:
        /**
        *@brief Constructor
        *@param initValue The current value of the semaphore, must be greater than zero.
        */
        CSemaphore(s32 initValue);


        /**
        *@brief Constructor
        *@param initValue The current value of the semaphore, range[0-max].
        *@param max The maximum value of the semaphore, must be greater than zero.
        */
        CSemaphore(s32 initValue, s32 max);

        /// Destructor
        ~CSemaphore();

        /**
        *@brief Increments the semaphore's value by one and
        * thus signals the semaphore. Another thread
        * waiting for the semaphore will be able to continue.
        */
        void set();

        /**
        *@brief Wait for the semaphore to become signalled.
        * To become signalled, a semaphore's value must be greater than zero. 
        * Decrements the semaphore's value by one.
        */
        void wait();

        /**
        *@brief Waits for the semaphore to become signalled.
        * To become signalled, a semaphore's value must be greater than zero.
        *@return true if the semaphore became signalled within the specified time interval, false otherwise.
        * Decrements the semaphore's value by one if successful.
        */
        bool wait(long milliseconds);


    private:
        CSemaphore();
        CSemaphore(const CSemaphore&);
        CSemaphore& operator = (const CSemaphore&);

#if defined(APP_PLATFORM_WINDOWS)
        void* mSema;    //HANDLE
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
        volatile s32 mValue;
        s32 mMax;
        pthread_mutex_t mMutex;
        pthread_cond_t  mCond;
#endif //APP_PLATFORM_WINDOWS
    };

} // namespace irr


#endif // APP_CREADWRITELOCK_H
