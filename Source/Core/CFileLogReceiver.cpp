#include "CFileLogReceiver.h"

#include <iostream>
#include <fstream>

namespace irr {

CFileLogReceiver::CFileLogReceiver() : IAntLogReceiver() {
    io::path fn(_IRR_TEXT("App.log"));
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


bool CFileLogReceiver::log(ELogLevel level, const c8* timestr, const c8* sender, const c8* message) {
    //c8 cache[1024];
    //u64 sz = snprintf(cache, sizeof(cache), "[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message);
    //mFile.write(cache, sz);

    return mFile.writeParams("[%s][%s] %s> %s\n", timestr, AppLogLevelNames[level], sender, message) > 0;
}

}//namespace irr 

