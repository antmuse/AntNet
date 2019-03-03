/**
*@file CThread.h
*@brief This file defined a thread that wrapped native threads of Windows and Linux.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef  APP_CTHREAD_H
#define  APP_CTHREAD_H

#include "HConfig.h"
#include "irrString.h"
#include "IRunnable.h"
#include "CMutex.h"
#include "CThreadEvent.h"

#if defined( APP_PLATFORM_WINDOWS )
#include <process.h>
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <pthread.h>
#include <signal.h>
#endif

namespace irr {

///A callable function pointer for threads.
typedef void(*AppCallable)(void*);


///A callable task for threads.
struct SThreadTask {
    enum ETaskType {
        ETT_NONE = 0,
        ETT_RUN,
        ETT_CALL
    };
    struct SCallbackData {
        AppCallable  mCallback;
        void* mData;
    };
    union {
        IRunnable* mCaller; //ETT_RUN
        SCallbackData mCallFunction;//ETT_CALL
    } mTarget;
    s32 mType;
    s32 mCount;
    SThreadTask* mNext;

    SThreadTask() {
        clear();
    }

    SThreadTask(IRunnable* target) {
        mType = SThreadTask::ETT_RUN;
        mTarget.mCaller = target;
        mCount = 0;
        mNext = 0;
    }

    SThreadTask(AppCallable iTarget, void* iData) {
        mType = SThreadTask::ETT_CALL;
        mTarget.mCallFunction.mCallback = iTarget;
        mTarget.mCallFunction.mData = iData;
        mCount = 0;
        mNext = 0;
    }

    SThreadTask& operator=(const SThreadTask& it) {
        ::memcpy(this, &it, sizeof(it));
        return *this;
    }

    SThreadTask& operator=(IRunnable& target) {
        mType = SThreadTask::ETT_RUN;
        mTarget.mCaller = &target;
        mCount = 1;
        return *this;
    }

    SThreadTask& setTarget(AppCallable iTarget, void* iData) {
        mType = SThreadTask::ETT_CALL;
        mTarget.mCallFunction.mCallback = iTarget;
        mTarget.mCallFunction.mData = iData;
        mCount = 1;
        return *this;
    }

    void clear() {
        ::memset(this, 0, sizeof(SThreadTask));
    }

    void operator()() {
        if(ETT_CALL == mType && mTarget.mCallFunction.mCallback) {
            mTarget.mCallFunction.mCallback(mTarget.mCallFunction.mData);
        } else if(ETT_RUN == mType && mTarget.mCaller) {
            mTarget.mCaller->run();
        }
    }
};

/**
*@class CThread
*@brief This class implements a platform-independent
* wrapper to an operating system thread.
*/
class CThread {
public:
#if defined( APP_PLATFORM_WINDOWS )
    ///Define Thread ID type.
    typedef DWORD TID;

#if defined(_DLL)
    typedef DWORD(WINAPI *AppThreadEntry)(LPVOID);
#else
    typedef unsigned(__stdcall *AppThreadEntry)(void*);
#endif

    /// Thread priorities.
    enum EThreadPriority {
        PRIO_LOWEST = THREAD_PRIORITY_LOWEST,					///< The lowest thread priority.
        PRIO_LOW = THREAD_PRIORITY_BELOW_NORMAL,    ///< A lower than normal thread priority.
        PRIO_NORMAL = THREAD_PRIORITY_NORMAL,			    ///< The normal thread priority.
        PRIO_HIGH = THREAD_PRIORITY_ABOVE_NORMAL,    ///< A higher than normal thread priority.
        PRIO_HIGHEST = THREAD_PRIORITY_HIGHEST					///< The highest thread priority.
    };


    enum EThreadPolicy {
        POLICY_DEFAULT = 0
    };
#endif		//APP_PLATFORM_WINDOWS


#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    ///Define Thread ID type.
    typedef pthread_t TID;

    /// Thread priorities.
    enum EThreadPriority {
        PRIO_LOWEST,					 ///< The lowest thread priority.
        PRIO_LOW,                           ///< A lower than normal thread priority.
        PRIO_NORMAL,			        ///< The normal thread priority.
        PRIO_HIGH,                         ///< A higher than normal thread priority.
        PRIO_HIGHEST					///< The highest thread priority.
    };

    enum EThreadPolicy {
        POLICY_DEFAULT = SCHED_OTHER
    };
#endif		//APP_PLATFORM_LINUX || APP_PLATFORM_ANDROID




    /// Creates a thread. Call start() to start it.
    CThread();


    /// Creates a named thread. Call start() to start it.
    CThread(const core::stringc& iName);


    /// Destructor
    ~CThread();


    /// Returns the unique thread ID of the thread.
    s32 getID() const {
        return mID;
    }


    /// Returns the native thread ID of the thread.
    TID getNativeID() const {
        return mThreadID;
    }


    /// Returns the name of the thread.
    const core::stringc& getName() const {
        return mName;
    }


    /// Sets the name of the thread.
    void setName(const core::stringc& iName);


    /**
    *@brief Sets the thread's priority.
    *@param iPriority Priority.
    */
    void setPriority(EThreadPriority iPriority);


    /// Returns the thread's priority.
    EThreadPriority getPriority() const {
        return EThreadPriority(mPriority);
    }


    /**
    *@brief Sets the thread's priority.
    *@param iPolicy Policy.
    *@return The mininum operating system-specific priority value by policy.
    */
    static s32 getMinPriority(s32 iPolicy = POLICY_DEFAULT);



    /**
    *@brief Sets the thread's priority.
    *@param iPolicy Policy.
    *@return The maxinum operating system-specific priority value by policy.
    */
    static s32 getMaxPriority(s32 iPolicy = POLICY_DEFAULT);


    /**
    *@brief Sets the thread's stack size in bytes.
    * Setting the stack size to 0 will use the OS's default stack size.
    * Typically, the real stack size is rounded up to the nearest page size multiple.
    *@param size Thread stack size in bytes.
    */
    void setStackSize(s32 size) {
        mStackSize = size;
    }


    /**
    *@brief Gets the thread's stack size.
    *@return The thread's stack size in bytes, or 0 if the default stack size is used.
    */
    s32 getStackSize() const {
        return mStackSize;
    }


    /**
    *@brief Start the thread with the given target.
    *@param target The runnable target.
    *@note The given runnable target must be
    * valid during the entire lifetime of the thread, as
    * only a reference to it is stored internally.
    */
    void start(IRunnable& target);


    /**
    *@brief Start the thread with the given target and parameter.
    *@param target The runnable target.
    *@param iData The parameter for runnable target.
    *@note The given runnable target must be
    * valid during the entire lifetime of the thread, as
    * only a reference to it is stored internally.
    */
    void start(AppCallable target, void* iData);


    /**
    *@brief Waitting until the thread completes execution.
    *@note If multiple threads try to join the same thread, the result is undefined.
    */
    void join();


    /**
    *@brief Wait for at most the given interval for the thread to complete.
    *@note If multiple threads try to join the same thread, the result is undefined.
    *@param milliseconds Wait time in millisecond.
    *@return false if the thread does not complete within the specified time interval, else true.
    */
    bool join(long milliseconds);


    /**
    *@return true if the thread is running.
    */
    bool isRunning() const;


    /**
    *@brief Wake up the thread which is in the state of interruptible sleep.
    *For threads that are not suspended, calling this
    * function has the effect of preventing the subsequent
    * wait() call to put thread in a suspended state.
    */
    void wakeUp();


    /**
    *@brief Starts an interruptible sleep. When wait() is called,
    * the thread will remain suspended until the timeout expires or wakeUp() is called.
    *
    *@note The wait() and wakeUp() calls should be used with
    * understanding that the suspended state is not a true sleep,
    * but rather a state of waiting for an event, with timeout
    * expiration. This makes order of calls significant; calling
    * wakeUp() before calling wait() will prevent the next
    * wait() call to actually suspend the thread (which, in
    * some scenarios, may be desirable behavior).
    *
    *@param milliseconds Wait time in millisecond.
    *@return true if sleep attempt was completed, false
    * if sleep was interrupted by a wakeUp() call.
    */
    static bool wait(long milliseconds);

    /**
    *@brief Let current thread wait for a given event for given times.
    *@param iEvent Wait event.
    *@param milliseconds Wait time in millisecond.
    *@return true if sleep attempt was completed, false
    * if sleep was interrupted by the given event's wakeUp() call.
    */
    static bool wait(CThreadEvent& iEvent, long milliseconds);

    /**
    *@brief Suspends the current thread for the specified amount of time.
    *@param milliseconds Sleep time in millisecond.
    */
    static void sleep(long milliseconds);


    /// Yields cpu to other threads.
    static void yield();


    /**
    *@return The CThread object for the currently active thread, or 0 if
    * the current thread is the main thread.
    */
    static CThread* getCurrentThread();


    /**
    *@return The native thread ID for the current thread.
    */
    static TID getCurrentNativeID();


protected:

    /// Creates a unique name for a thread.
    void makeName();

    /**
    *@brief Creates a unique id for a thread.
    *@return A unique id for a thread.
    */
    static s32 getUniqueID();


#if defined( APP_PLATFORM_WINDOWS )
#if defined(_DLL)
    static DWORD WINAPI runnableEntry(LPVOID iThread);
    static DWORD WINAPI callableEntry(LPVOID iThread);
#else
    static unsigned __stdcall runnableEntry(void* iThread);
    static unsigned __stdcall callableEntry(void* iThread);
#endif

    void createThread(AppThreadEntry ent, void* iData);

    void threadCleanup();
#endif		//APP_PLATFORM_WINDOWS


#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    static void* runnableEntry(void* iThread);
    static void* callableEntry(void* iThread);
    static s32 mapPrio(s32 iPriority, s32 iPolicy = POLICY_DEFAULT);
#endif		//APP_PLATFORM_LINUX || APP_PLATFORM_ANDROID



private:
    CThread(const CThread& other) = delete;
    CThread& operator=(const CThread& other) = delete;


    s32 mID;
    s32 mPriority;
    s32 mPolicy;
    s32 mStackSize;
    TID mThreadID;
    core::stringc mName;
    CThreadEvent mEvent;
    SThreadTask mTask;

#if defined( APP_PLATFORM_WINDOWS )
    class CCurrentThreadHolder {
    public:
        CCurrentThreadHolder() : mSlot(TlsAlloc()) {
            if(mSlot == TLS_OUT_OF_INDEXES) {
                //("cannot allocate thread context key");
            }
        }
        ~CCurrentThreadHolder() {
            TlsFree(mSlot);
        }
        CThread* get() const {
            return reinterpret_cast<CThread*>(TlsGetValue(mSlot));
        }
        void set(CThread* iThread) {
            TlsSetValue(mSlot, iThread);
        }//CCurrentThreadHolder

    private:
        DWORD mSlot;
    };

    void* mThread; //HANDLE
#endif		//APP_PLATFORM_WINDOWS


#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    class CCurrentThreadHolder {
    public:
        CCurrentThreadHolder() {
            if(pthread_key_create(&_key, NULL)) {
                //("cannot allocate thread context key");
            }
        }
        ~CCurrentThreadHolder() {
            pthread_key_delete(_key);
        }
        CThread* get() const {
            return reinterpret_cast<CThread*>(pthread_getspecific(_key));
        }
        void set(CThread* iThread) {
            pthread_setspecific(_key, iThread);
        }
    private:
        pthread_key_t _key;
    };

#endif		//APP_PLATFORM_LINUX || APP_PLATFORM_ANDROID


    static CCurrentThreadHolder mCurrentHolder;
};



} //end namespace irr


#endif  //APP_CTHREAD_H