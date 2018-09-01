#include "CConsoleLogReceiver.h"
#include <stdio.h>

#if defined(APP_PLATFORM_ANDROID)
#include <android/log.h>
#endif

#ifdef APP_COMPILE_WITH_CONSOLE_LOG_RECEIVER

namespace irr {


CConsoleLogReceiver::CConsoleLogReceiver() : IAntLogReceiver() {
}



CConsoleLogReceiver::~CConsoleLogReceiver() {
}



bool CConsoleLogReceiver::log(ELogLevel level, const c8* pSender, const c8* pMessage) {
#if defined(APP_PLATFORM_ANDROID)
    __android_log_print(level + ANDROID_LOG_DEBUG, "NeatSpark", "[%s]  %s>    %s\n", AppLogLevelNames[level], pSender, pMessage);
#else
    printf("[%s]  %s>    %s\n", AppLogLevelNames[level], pSender, pMessage);
#endif
    return true;
}



bool CConsoleLogReceiver::log(ELogLevel level, const wchar_t* pSender, const wchar_t* pMessage) {
#if defined(APP_PLATFORM_ANDROID)
    __android_log_print(level + ANDROID_LOG_DEBUG, "NeatSpark", "[%s]  %s>    %s\n", AppLogLevelNames[level], pSender, pMessage);
#else
    wprintf(L"[%s]  %s>    %s\n", AppWLogLevelNames[level], pSender, pMessage);
#endif
    return true;
}



}//namespace irr 

#endif //APP_COMPILE_WITH_CONSOLE_LOG_RECEIVER



