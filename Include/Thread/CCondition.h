/**
*@file CCondition.h
*@brief This file defined thread event.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_CCONDITION_H
#define APP_CCONDITION_H

#include "HConfig.h"
#include "irrTypes.h"

#if defined( APP_PLATFORM_WINDOWS )
#include <winsock2.h>   //just here to prevent <winsock.h>
#include <Windows.h>
#endif
#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <pthread.h>
#include <errno.h>
#endif

namespace irr {
class CMutex;

/**
*@class CCondition
*@brief A CCondition is a synchronization object used for threads, not processes.
*/
class  CCondition {
public:
    /**
    *@brief Constructor
    */
    CCondition();

    ~CCondition();

    /**
    *@brief init condition.
    *@param super True to used between processes, else for threads.
    *@return True if init success, else false.
    */
    //bool init(bool super);

    /**
    *@brief Wake up one of the threads witch waiting for this condition.
    *@note notify between lock and unlock or notify after unlock is defined by user.
    *@return true if success, else false.
    */
    bool notify();

    /**
    *@brief Wake up all threads witch waiting for this condition.
    *@note notify between lock and unlock or notify after unlock is defined by user.
    *@return true if success, else false.
    */
    bool notifyAll();

    /**
    *@brief Wait for this condition for 0xFFFFFFFF milliseconds.
    *@param mutex Mutex used by condition.
    *@note The mutex must had been locked first.
    *@return true if success, else false.
    */
    bool wait(CMutex& mutex);

    /**
    *@brief Wait for this condition for some milliseconds.
    *@param mutex Mutex used by condition.
    *@param milliseconds Waiting time, in milliseconds.
    *@note The mutex must had been locked first.
    *@return true if success, else false.
    */
    bool wait(CMutex& mutex, u32 milliseconds);

private:
#if defined(APP_PLATFORM_WINDOWS)
    CONDITION_VARIABLE mCondition;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    pthread_cond_t mCondition;
    bool init();
#endif
};

//class CSuperCondition;

} //end namespace irr

#endif	/* APP_CCONDITION_H */