#include "CFileLogReceiver.h"
#include "StrConverter.h"

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

    s8 s8cache[1024];
    usz wsz = core::AppWcharToUTF8(wcache, s8cache, sizeof(s8cache));
    return mFile.write(s8cache, wsz) > 0;
}


bool CFileLogReceiver::log(ELogLevel level, const s8* timestr, const s8* sender, const s8* message) {
    //s8 cache[1024];
    //u64 sz = snprintf(cache, sizeof(cache), "[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message);
    //mFile.write(cache, sz);

    return mFile.writeParams("[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message) > 0;
}

}//namespace app 

