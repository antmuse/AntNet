#include "CConsoleLogReceiver.h"
#include <stdio.h>
#include <wchar.h>
#if defined(APP_PLATFORM_ANDROID)
#include <android/log.h>
#endif


namespace app {


CConsoleLogReceiver::CConsoleLogReceiver() : ILogReceiver() {
}



CConsoleLogReceiver::~CConsoleLogReceiver() {
}



bool CConsoleLogReceiver::log(ELogLevel level, const s8* timestr, const s8* pSender, const s8* pMessage) {
#if defined(APP_PLATFORM_ANDROID)
    __android_log_print(level + ANDROID_LOG_DEBUG, "NeatSpark", "[%s] %s> %s\n", AppLogLevelNames[level], pSender, pMessage);
#else
    printf("[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], pSender, pMessage);
#endif
    return true;
}



bool CConsoleLogReceiver::log(ELogLevel level, const wchar_t* timestr, const wchar_t* pSender, const wchar_t* pMessage) {
#if defined(APP_PLATFORM_ANDROID)
    __android_log_print(level + ANDROID_LOG_DEBUG, "NeatSpark", "[%s] %s> %s\n", AppLogLevelNames[level], pSender, pMessage);
#else
    wprintf(L"[%s][%s] %s> %s\n", timestr, AppWLogLevelNames[level], pSender, pMessage);
#endif
    return true;
}



}//namespace app



