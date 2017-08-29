/**
*@file CProcessHandle.h
*@brief This file defined process handle.
*@author antmuse@live.cn
*@date 2014-10-22
*/


#ifndef APP_CPROCESSHANDLE_H
#define APP_CPROCESSHANDLE_H

#include "HConfig.h"
#include "IReferenceCounted.h"


namespace irr {
    /**
    *@class CProcessHandle
    *@brief A handle for a process created with CProcessManager::launch().
    *
    * This handle can be used to determine the process ID of
    * the newly created process and it can be used to wait for
    * the completion of a process.
    */
    class CProcessHandle : public IReferenceCounted{
    public:
#if defined(APP_PLATFORM_WINDOWS)
        typedef u32 PID;
        CProcessHandle(void* handle, PID id);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        typedef s32 PID;
        CProcessHandle(PID id);
#endif //APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID


        /// Destructor.
        ~CProcessHandle();

        /**
        *@brief Get process ID.
        *@return The process ID.
        */
        PID getID() const;

        /**
        *@brief Wait for the process to terminate.
        *@return The exit code of the process.
        */
        s32 wait() const;


#if defined(APP_PLATFORM_WINDOWS)
        void* getHandle() const;
        void closeHandle();
#endif //APP_PLATFORM_WINDOWS


    private:
        CProcessHandle();

        /// Prevent creates a CProcessHandle by copying another one.
        CProcessHandle(const CProcessHandle& it);

        /// Prevent assigns an other process handle.
        CProcessHandle& operator = (const CProcessHandle& it);


        PID mID;
#if defined(APP_PLATFORM_WINDOWS)
        void* mProcess;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#endif //APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID
    };


} //end namespace irr

#endif	/* APP_CPROCESSHANDLE_H */
