#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include "IAppLogger.h"
#include "CMutex.h"


namespace irr {

CMutex AppLogMutex;


c8 IAppLogger::mTextBuffer[MAX_TEXT_BUFFER_SIZE];
wchar_t IAppLogger::mTextBufferW[MAX_TEXT_BUFFER_SIZE];


#if defined(APP_DEBUG)
ELogLevel IAppLogger::mMinLogLevel = ELOG_DEBUG;
#else
ELogLevel IAppLogger::mMinLogLevel = ELOG_INFO;
#endif



IAppLogger::IAppLogger() {

}


IAppLogger::~IAppLogger() {
}


void IAppLogger::setLevel(ELogLevel iLevel) {
    mMinLogLevel = iLevel;
}


void IAppLogger::log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iFormat, ...) {
    if(iLevel >= mMinLogLevel) {
        CAutoLock aulock(AppLogMutex);
        va_list args;
        va_start(args, iFormat);
#if defined(APP_PLATFORM_WINDOWS)
        _vsnwprintf(mTextBufferW, MAX_TEXT_BUFFER_SIZE, iFormat, args);
        wprintf(L"[%s]  %s >> %s\n", AppLogLevelStringsW[iLevel], iSender, mTextBufferW);
#elif defined(APP_PLATFORM_LINUX)
        vswprintf(mTextBufferW, MAX_TEXT_BUFFER_SIZE, iFormat, args);
        printf("[%ls]  %ls >> %ls\n", AppLogLevelStringsW[iLevel], iSender, mTextBufferW);
#endif //
        va_end(args);
    }
}


void IAppLogger::log(ELogLevel iLevel, const c8* iSender, const c8* iFormat, ...) {
    if(iLevel >= mMinLogLevel) {
        CAutoLock aulock(AppLogMutex);
        va_list args;
        va_start(args, iFormat);
        vsnprintf(mTextBuffer, MAX_TEXT_BUFFER_SIZE, iFormat, args);
        printf("[%s]  %s >> %s\n", AppLogLevelStrings[iLevel], iSender, mTextBuffer);
        va_end(args);
    }
}


}//namespace irr
