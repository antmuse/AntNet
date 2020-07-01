#include "IUtility.h"
#include "CLogger.h"
#include "CTimer.h"
#include "CFileManager.h"
#include "CConsoleLogReceiver.h"
#include "CFileLogReceiver.h"
#include "CHtmlLogReceiver.h"

namespace app {


s8 CLogger::mTextBuffer[MAX_TEXT_BUFFER_SIZE];
wchar_t CLogger::mTextBufferW[MAX_TEXT_BUFFER_SIZE];
core::TArray<ILogReceiver*> CLogger::mAllReceiver;
CMutex CLogger::mMutex;

#ifdef   APP_DEBUG
ELogLevel CLogger::mMinLogLevel = ELOG_DEBUG;
#else       //for release version
ELogLevel CLogger::mMinLogLevel = ELOG_INFO;
#endif  //release version


CLogger::CLogger() {
#if defined(APP_USE_ICONV)
    core::IUtility::getInstance();
#endif
    io::CFileManager::getStartWorkPath();
}


CLogger::~CLogger() {
    clear();
}


CLogger& CLogger::getInstance() {
    static CLogger it;
    return it;
}

void CLogger::addReceiver(u32 flag) {
    if(CLogger::ELRT_CONSOLE & flag) {
        CLogger::add(new CConsoleLogReceiver());
    }
    if(CLogger::ELRT_FILE_TEXT & flag) {
        CLogger::add(new CFileLogReceiver());
    }
    if(CLogger::ELRT_FILE_HTML & flag) {
        CLogger::add(new CHtmlLogReceiver());
    }
}

void CLogger::log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMsg, ...) {
    if(iLevel >= mMinLogLevel) {
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iSender, iMsg, args);
        va_end(args);
    }
}
void CLogger::log(ELogLevel iLevel, const s8* iSender, const s8* iMsg, ...) {
    if(iLevel >= mMinLogLevel) {
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iSender, iMsg, args);
        va_end(args);
    }
}

void CLogger::logCritical(const s8* sender, const s8* msg, ...) {
    if(ELOG_CRITICAL >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_CRITICAL, sender, msg, args);
        va_end(args);
    }
}
void CLogger::logCritical(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_CRITICAL >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_CRITICAL, sender, msg, args);
        va_end(args);
    }
}


void CLogger::logError(const s8* sender, const s8* msg, ...) {
    if(ELOG_ERROR >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_ERROR, sender, msg, args);
        va_end(args);
    }
}
void CLogger::logError(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_ERROR >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_ERROR, sender, msg, args);
        va_end(args);
    }
}


void CLogger::logWarning(const s8* sender, const s8* msg, ...) {
    if(ELOG_WARN >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_WARN, sender, msg, args);
        va_end(args);
    }
}
void CLogger::logWarning(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_WARN >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_WARN, sender, msg, args);
        va_end(args);
    }
}


void CLogger::logInfo(const s8* sender, const s8* msg, ...) {
    if(ELOG_INFO >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_INFO, sender, msg, args);
        va_end(args);
    }
}
void CLogger::logInfo(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_INFO >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_INFO, sender, msg, args);
        va_end(args);
    }
}


void CLogger::logDebug(const s8* sender, const s8* msg, ...) {
    if(ELOG_DEBUG >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_DEBUG, sender, msg, args);
        va_end(args);
    }
}
void CLogger::logDebug(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_DEBUG >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_DEBUG, sender, msg, args);
        va_end(args);
    }
}


void CLogger::setLogLevel(const ELogLevel logLevel) {
    mMinLogLevel = logLevel;
}

void CLogger::postLog(ELogLevel level, const s8* sender, const s8* msg, va_list args) {
    if(sender && msg) {
        CAutoLock ak(mMutex);
        s8 tmstr[20];
        CTimer::getTimeAsString(tmstr, sizeof(tmstr));
        vsnprintf(mTextBuffer, MAX_TEXT_BUFFER_SIZE, msg, args);
        for(u32 it = 0; it < mAllReceiver.size(); ++it) {
            mAllReceiver[it]->log(level, tmstr, sender, mTextBuffer);
        }
    }
}

void CLogger::postLog(ELogLevel level, const wchar_t* sender, const wchar_t* msg, va_list args) {
    if(sender && msg) {
        CAutoLock ak(mMutex);
        wchar_t tmstr[20];
        CTimer::getTimeAsString(tmstr, sizeof(tmstr));
#if defined(APP_PLATFORM_WINDOWS)
        _vsnwprintf(mTextBufferW, MAX_TEXT_BUFFER_SIZE, msg, args);
        for(u32 it = 0; it < mAllReceiver.size(); ++it) {
            mAllReceiver[it]->log(level, tmstr, sender, mTextBufferW);
        }
#endif
    }
}


bool CLogger::add(ILogReceiver* pReceiver) {
    if(pReceiver) {
        CAutoLock ak(mMutex);
        mAllReceiver.pushBack(pReceiver);
    }
    return true;
}


void CLogger::remove(const ILogReceiver* iLog) {
    CAutoLock ak(mMutex);
    for(u32 it = 0; it < mAllReceiver.size(); ++it) {
        if(iLog == mAllReceiver[it]) {
            delete mAllReceiver[it];
            mAllReceiver.erase(it);
            break;
        }
    }
}


void CLogger::clear() {
    CAutoLock ak(mMutex);
    for(u32 it = 0; it < mAllReceiver.size(); ++it) {
        delete mAllReceiver[it];
    }
    mAllReceiver.setUsed(0);
}


}//namespace app
