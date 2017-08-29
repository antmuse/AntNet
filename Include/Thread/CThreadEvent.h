/**
*@file CThreadEvent.h
*@brief This file defined thread event.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_CTHREADEVENT_H
#define APP_CTHREADEVENT_H

#include "HConfig.h"
#include "irrTypes.h"

#if defined(APP_PLATFORM_LINUX)
#include <pthread.h>
#include <errno.h>
#endif

namespace irr {


    /**
    *@class CThreadEvent
    *@brief An CThreadEvent is a synchronization object that
    * allows one thread to signal one or more
    * other threads that a certain event has happened.
    * Usually, one thread signals an event,
    * while one or more other threads wait
    * for an event to become signalled.
    */
    class  CThreadEvent {
    public:
        /**
        *@brief Creates an event. 
        *@param iName Event name.
        *@param autoReset The event is automatically reset after
        * a wait() successfully returns if ture, else not reset.
        */
        CThreadEvent(const fschar_t* iName, bool autoReset);


        /// Destructor
        ~CThreadEvent();

        /**
        *@brief Signals the event. If autoReset is true,
        * only one thread waiting for the event 
        * can resume execution.
        * If autoReset is false, all waiting threads can resume execution.
        */
        void set();

        /// Resets the event to unsignalled state.
        void reset();

        /// Waits for the event to become signalled.
        bool wait();

        /**
        *@brief  Waits for the event to become signalled.
        *@parma milliseconds Wait time in millisecond.
        *@return false if the event does not become signalled within the specified time interval,
        * else true.
        */
        bool wait(long milliseconds);


        //bool tryWait(long milliseconds);


    private:
        CThreadEvent();
        CThreadEvent(const CThreadEvent&);
        CThreadEvent& operator = (const CThreadEvent&);

#if defined(APP_PLATFORM_WINDOWS)
        void* mHandle;
#elif defined(APP_PLATFORM_LINUX)
        volatile bool   mStatus;
        bool            mAutoReset;
        pthread_cond_t mCond;
        pthread_mutex_t mMutex;
#endif
    };


} //end namespace irr

#endif	/* APP_CTHREADEVENT_H */