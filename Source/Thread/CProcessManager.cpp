#include "CProcessManager.h"
#include "CPipe.h"
#include "CThreadEvent.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif //APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID


namespace irr {

core::array<fschar_t> CProcessManager::getEnvironmentVariablesBuffer(const CProcessManager::DProcessEnvronment& env) {
    core::array<fschar_t> envbuf;
    u32 pos = 0;
    u32 keysize;
    u32 valuesize;
    for(CProcessManager::DProcessEnvronment::ConstIterator it = env.getConstIterator(); !it.atEnd(); it++) {
        keysize = it.getNode()->getKey().size();
        valuesize = it.getNode()->getValue().size();
        envbuf.reallocate(pos + 2 + valuesize + keysize); //note: 2byte = 1 byte for last char '\0' and 1 byte for char '=';
        memcpy(envbuf.pointer() + pos, it.getNode()->getKey().c_str(), keysize);
        pos += keysize;
        envbuf[pos++] = '=';    //1 byte
        memcpy(envbuf.pointer() + pos, it.getNode()->getValue().c_str(), valuesize);
        pos += valuesize;
        envbuf[pos++] = '\0';   //1 byte
    }//for

    envbuf.set_used(pos);
    envbuf.push_back('\0');

    APP_ASSERT(pos + 1 == envbuf.size());

    return envbuf;
}


CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args) {
    io::path initialDirectory;
    DProcessEnvronment env;
    return (launch(command, args, initialDirectory, 0, 0, 0, env));
}


CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args, const io::path& initialDirectory) {
    DProcessEnvronment env;
    return (launch(command, args, initialDirectory, 0, 0, 0, env));
}


CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args, CPipe* inPipe, CPipe* outPipe, CPipe* errPipe) {
    APP_ASSERT(inPipe == 0 || (inPipe != outPipe && inPipe != errPipe));
    io::path initialDirectory;
    DProcessEnvronment env;
    return (launch(command, args, initialDirectory, inPipe, outPipe, errPipe, env));
}


CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args, const io::path& initialDirectory,
    CPipe* inPipe, CPipe* outPipe, CPipe* errPipe) {
    APP_ASSERT(inPipe == 0 || (inPipe != outPipe && inPipe != errPipe));
    DProcessEnvronment env;
    return (launch(command, args, initialDirectory, inPipe, outPipe, errPipe, env));
}


CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args, CPipe* inPipe,
    CPipe* outPipe, CPipe* errPipe, const DProcessEnvronment& env) {
    APP_ASSERT(inPipe == 0 || (inPipe != outPipe && inPipe != errPipe));
    io::path initialDirectory;
    return (launch(command, args, initialDirectory, inPipe, outPipe, errPipe, env));
}


} //namespace irr




#if defined(APP_PLATFORM_WINDOWS)
namespace irr {

CProcessHandle::PID CProcessManager::getCurrentID() {
    return ::GetCurrentProcessId();
}


void CProcessManager::times(long& userTime, long& kernelTime) {
    FILETIME ftCreation;
    FILETIME ftExit;
    FILETIME ftKernel;
    FILETIME ftUser;

    if(::GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit, &ftKernel, &ftUser) != 0) {
        ULARGE_INTEGER time;
        time.LowPart = ftKernel.dwLowDateTime;
        time.HighPart = ftKernel.dwHighDateTime;
        kernelTime = long(time.QuadPart / 10000000L);
        time.LowPart = ftUser.dwLowDateTime;
        time.HighPart = ftUser.dwHighDateTime;
        userTime = long(time.QuadPart / 10000000L);
    } else {
        userTime = kernelTime = -1;
    }
}


static bool needEscaping(const io::path& arg) {
    bool isAlreadyQuoted = (arg.size() >= 2) &&
        (_IRR_TEXT('\"') == arg[0]) &&
        (_IRR_TEXT('\"') == arg[arg.size() - 1]);

    if(isAlreadyQuoted) {
        return false;
    }

    bool ret = (arg.findFirst(_IRR_TEXT(' ')) >= 0) ||
        (arg.findFirst(_IRR_TEXT('\t')) >= 0) ||
        (arg.findFirst(_IRR_TEXT('\n')) >= 0) ||
        (arg.findFirst(_IRR_TEXT('\v')) >= 0) ||
        (arg.findFirst(_IRR_TEXT('\"')) >= 0);

    return ret;
}


static io::path escapeParam(const io::path& arg) {
    if(!needEscaping(arg)) {
        return arg;
    }

    io::path ret("\"");
    ret.reserve(arg.size() * 11 / 10 + 6);

    for(u32 it = 0; it < arg.size(); ++it) {
        switch(arg[it]) {
        case '\\':
            ret.append('\\');
            ret.append('\\');
            break;
        case '\"':
            ret.append('\\');
            ret.append('\"');
            break;
        default:
            ret.append(arg[it]);
            break;
        }//switch
    }//for

    ret.append('\"');
    return ret;
}



CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args, const io::path& initialDirectory,
    CPipe* inPipe, CPipe* outPipe, CPipe* errPipe, const DProcessEnvronment& env) {
    io::path commandLine(command);
    for(u32 i = 0; i < args.size(); ++i) {
        commandLine.append(_IRR_TEXT(' '));
        commandLine.append(escapeParam(args[i]));
    }

    STARTUPINFO startupInfo;
    ::GetStartupInfo(&startupInfo); // take defaults from current process
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpReserved = NULL;
    startupInfo.lpDesktop = NULL;
    startupInfo.lpTitle = NULL;
    startupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = NULL;

    HANDLE hProc = ::GetCurrentProcess();
    bool mustInheritHandles = false;
    if(inPipe) {
        ::DuplicateHandle(hProc, inPipe->getReadHandle(), hProc, &startupInfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
        inPipe->closeRead();
    } else if(GetStdHandle(STD_INPUT_HANDLE)) {
        ::DuplicateHandle(hProc, GetStdHandle(STD_INPUT_HANDLE), hProc, &startupInfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
    } else {
        startupInfo.hStdInput = 0;
    }

    // outPipe may be the same as errPipe, so we duplicate first and close later.
    if(outPipe) {
        ::DuplicateHandle(hProc, outPipe->getWriteHandle(), hProc, &startupInfo.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
    } else if(GetStdHandle(STD_OUTPUT_HANDLE)) {
        ::DuplicateHandle(hProc, GetStdHandle(STD_OUTPUT_HANDLE), hProc, &startupInfo.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
    } else {
        startupInfo.hStdOutput = 0;
    }

    if(errPipe) {
        ::DuplicateHandle(hProc, errPipe->getWriteHandle(), hProc, &startupInfo.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
    } else if(GetStdHandle(STD_ERROR_HANDLE)) {
        ::DuplicateHandle(hProc, GetStdHandle(STD_ERROR_HANDLE), hProc, &startupInfo.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
    } else {
        startupInfo.hStdError = 0;
    }

    if(outPipe) outPipe->closeWrite();

    if(errPipe) errPipe->closeWrite();

    if(mustInheritHandles) {
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    const fschar_t* workingDirectory = initialDirectory.empty() ? 0 : initialDirectory.c_str();

    const fschar_t* pEnv = 0;

    core::array<fschar_t> envChars;

    if(!env.empty()) {
        envChars = getEnvironmentVariablesBuffer(env);
        pEnv = &envChars[0];
    }

    PROCESS_INFORMATION processInfo;
    DWORD creationFlags = ::GetConsoleWindow() ? 0 : CREATE_NO_WINDOW;
    BOOL rc = ::CreateProcess(
        0,
        const_cast<fschar_t*>(commandLine.c_str()),
        0, // process Attributes
        0, // thread Attributes
        mustInheritHandles,
        creationFlags,
        (LPVOID) pEnv,
        workingDirectory,
        &startupInfo,
        &processInfo
    );

    if(startupInfo.hStdInput) ::CloseHandle(startupInfo.hStdInput);

    if(startupInfo.hStdOutput) ::CloseHandle(startupInfo.hStdOutput);

    if(startupInfo.hStdError) ::CloseHandle(startupInfo.hStdError);

    if(rc) {
        ::CloseHandle(processInfo.hThread);
        return new CProcessHandle(processInfo.hProcess, processInfo.dwProcessId);
    }

    return 0; //printf("Cannot launch process: [%s]\n", command);
}


void CProcessManager::kill(CProcessHandle& handle) {
    if(handle.getHandle()) {
        if(0 == ::TerminateProcess(handle.getHandle(), 0)) {
            //("cannot kill process");
        }
        handle.closeHandle();
    }
}


void CProcessManager::kill(CProcessHandle::PID pid) {
    HANDLE hProc = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if(hProc) {
        if(0 == ::TerminateProcess(hProc, 0)) {
            // ("cannot kill process");
        }
        ::CloseHandle(hProc);
    } else { //Had not got process by ID
        switch(::GetLastError()) {
        case ERROR_ACCESS_DENIED:
            break;
        case ERROR_NOT_FOUND:
            break;
        case ERROR_INVALID_PARAMETER:
            break;
        default:
            break;
        }
    }
}


bool CProcessManager::isRunning(const CProcessHandle& handle) {
    bool result = true;
    DWORD exitCode;
    BOOL rc = ::GetExitCodeProcess(handle.getHandle(), &exitCode);
    if(!rc || exitCode != STILL_ACTIVE) {
        result = false;
    }
    return result;
}


bool CProcessManager::isRunning(CProcessHandle::PID pid) {
    HANDLE hProc = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    bool result = true;
    DWORD exitCode;
    BOOL rc = ::GetExitCodeProcess(hProc, &exitCode);

    if(!rc || exitCode != STILL_ACTIVE) {
        result = false;
    }
    return result;
}

} //end namespace irr
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
namespace irr {

CProcessHandle::PID CProcessManager::getCurrentID() {
    return ::getpid();
}


void CProcessManager::times(long& userTime, long& kernelTime) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    userTime = usage.ru_utime.tv_sec;
    kernelTime = usage.ru_stime.tv_sec;
}


CProcessHandle* CProcessManager::launch(const io::path& command, const DProcessParam& args, const io::path& initialDirectory,
    CPipe* inPipe, CPipe* outPipe, CPipe* errPipe, const DProcessEnvronment& env) {
    // We must not allocated memory after fork(), therefore allocate all required buffers first.
    core::array<fschar_t> envChars = getEnvironmentVariablesBuffer(env);
    core::array<fschar_t*> argv(args.size() + 2);
    s32 i = 0;
    argv[i++] = const_cast<fschar_t*>(command.c_str());
    for(u32 it = 0; it < args.size(); ++it) {
        argv[i++] = const_cast<fschar_t*>(args[it].c_str());
    }
    argv[i] = 0;

    const fschar_t* pInitialDirectory = initialDirectory.empty() ? 0 : initialDirectory.c_str();

    s32 pid = fork();
    if(0 != pid) {
        return 0; //printf("Cannot fork process for", command);
    }


    if(pInitialDirectory) {
        if(chdir(pInitialDirectory) != 0) {
            _exit(72);
        }
    }

    // set environment variables
    fschar_t* p = &envChars[0];
    while(*p) {
        putenv(p);
        while(*p) {
            ++p;
        }
        ++p;
    }

    // setup redirection
    if(inPipe) {
        dup2(inPipe->getReadHandle(), STDIN_FILENO);
        inPipe->closeAll();
    }

    // outPipe and errPipe may be the same, so we dup first and close later
    if(outPipe) dup2(outPipe->getWriteHandle(), STDOUT_FILENO);

    if(errPipe) dup2(errPipe->getWriteHandle(), STDERR_FILENO);

    if(outPipe) outPipe->closeAll();

    if(errPipe) errPipe->closeAll();

    // close all open file descriptors other than stdin, stdout, stderr
    for(s32 i = 3; i < sysconf(_SC_OPEN_MAX); ++i) {
        close(i);
    }

    execvp(argv[0], &argv[0]);
    _exit(72);

    if(inPipe)  inPipe->closeRead();

    if(outPipe) outPipe->closeWrite();

    if(errPipe) errPipe->closeWrite();

    return new CProcessHandle(pid);
}


void CProcessManager::kill(CProcessHandle& handle) {
    CProcessManager::kill(handle.getID());
}


void CProcessManager::kill(CProcessHandle::PID pid) {
    if(::kill(pid, SIGKILL) != 0) { //("cannot kill process");
        switch(errno) {
        case ESRCH:
            break;
        case EPERM:
            break;
        default:
            break;
        }
    }
}


bool CProcessManager::isRunning(const CProcessHandle& handle) {
    return isRunning(handle.getID());
}


bool CProcessManager::isRunning(CProcessHandle::PID pid) {
    if(::kill(pid, 0) == 0) {
        return true;
    } else {
        return false;
    }
}

/*
void CProcessManager::requestTermination(CProcessHandle::PID pid){
if (::kill(pid, SIGINT) != 0){
switch (errno)	{
case ESRCH:
throw NotFoundException("cannot terminate process");
case EPERM:
throw NoPermissionException("cannot terminate process");
default:
throw SystemException("cannot terminate process");
}
}
}
*/

} //end namespace irr
#endif //APP_PLATFORM_LINUX & APP_PLATFORM_ANDROID


