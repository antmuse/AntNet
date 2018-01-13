#include "CProcessHandle.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif //APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID



#if defined(APP_PLATFORM_WINDOWS)
namespace irr {

CProcessHandle::CProcessHandle(void* hProcess, CProcessHandle::PID pid) : mProcess(hProcess),
mID(pid) {
}


CProcessHandle::~CProcessHandle() {
    closeHandle();
}


void CProcessHandle::closeHandle() {
    if(mProcess) {
        ::CloseHandle(mProcess);
        mProcess = 0;
    }
}


/*CProcessHandle& CProcessHandle::operator = (const CProcessHandle& handle) {
    if (&handle != this) {
    }
    return *this;
}*/


CProcessHandle::PID CProcessHandle::getID() const {
    return mID;
}


void* CProcessHandle::getHandle() const {
    return mProcess;
}


s32 CProcessHandle::wait() const {
    DWORD rc = ::WaitForSingleObject(mProcess, INFINITE);
    if(rc != WAIT_OBJECT_0) {
        return -1; //"Wait failed for process"
    }

    DWORD exitCode;

    if(::GetExitCodeProcess(mProcess, &exitCode) == 0) {
        return -1; //"Cannot get exit code for process"
    }
    return exitCode;
}


} //end namespace irr
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace irr {

CProcessHandle::CProcessHandle(CProcessHandle::PID pid) : mID(pid) {
}


CProcessHandle::~CProcessHandle() {
}


CProcessHandle::PID CProcessHandle::getID() const {
    return mID;
}


s32 CProcessHandle::wait() const {
    s32 status;
    s32 rc;
    do {
        rc = waitpid(mID, &status, 0);
    } while(rc < 0 && errno == EINTR);

    if(rc != mID) {
        //("Cannot wait for process", NumberFormatter::format(mID));
    }
    return WEXITSTATUS(status);
}

} //end namespace irr
#endif //APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID


