#include "CFileLogReceiver.h"

#include <iostream>
#include <fstream>

namespace app {

CFileLogReceiver::CFileLogReceiver() : ILogReceiver() {
    core::CPath fn(APP_STR("App.log"));
    mFile.openFile(fn, true);
}

CFileLogReceiver::~CFileLogReceiver() {
}


bool CFileLogReceiver::log(ELogLevel level, const wchar_t* timestr, const wchar_t* sender, const wchar_t* message) {
    wchar_t wcache[1024];
    swprintf(wcache, 1024, L"[%s][%s] %s> %s\n", timestr, AppWLogLevelNames[level], sender, message);
    //mFile.writeWParams(L"[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message);
    return true;
}


bool CFileLogReceiver::log(ELogLevel level, const s8* timestr, const s8* sender, const s8* message) {
    //s8 cache[1024];
    //u64 sz = snprintf(cache, sizeof(cache), "[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message);
    //mFile.write(cache, sz);

    return mFile.writeParams("[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message) > 0;
}

}//namespace app 

