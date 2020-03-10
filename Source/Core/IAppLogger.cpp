#include "IUtility.h"
#include "IAppLogger.h"
#include "IAppTimer.h"
#include "CFileManager.h"
#include "CConsoleLogReceiver.h"
#include "CFileLogReceiver.h"
#include "CHtmlLogReceiver.h"

namespace irr {


c8 IAppLogger::mTextBuffer[MAX_TEXT_BUFFER_SIZE];
wchar_t IAppLogger::mTextBufferW[MAX_TEXT_BUFFER_SIZE];
core::array<IAntLogReceiver*> IAppLogger::mAllReceiver;
CMutex IAppLogger::mMutex;

#ifdef   APP_DEBUG
ELogLevel IAppLogger::mMinLogLevel = ELOG_DEBUG;
#else       //for release version
ELogLevel IAppLogger::mMinLogLevel = ELOG_INFO;
#endif  //release version


IAppLogger::IAppLogger() {
    core::IUtility::getInstance();
    io::CFileManager::getStartWorkPath();
}


IAppLogger::~IAppLogger() {
    clear();
}


IAppLogger& IAppLogger::getInstance() {
    static IAppLogger it;
    return it;
}

void IAppLogger::addReceiver(u32 flag) {
    if(IAppLogger::ELRT_CONSOLE & flag) {
        IAppLogger::add(new CConsoleLogReceiver());
    }
    if(IAppLogger::ELRT_FILE_TEXT & flag) {
        IAppLogger::add(new CFileLogReceiver());
    }
    if(IAppLogger::ELRT_FILE_HTML & flag) {
        IAppLogger::add(new CHtmlLogReceiver());
    }
}

void IAppLogger::log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMsg, ...) {
    if(iLevel >= mMinLogLevel) {
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iSender, iMsg, args);
        va_end(args);
    }
}
void IAppLogger::log(ELogLevel iLevel, const c8* iSender, const c8* iMsg, ...) {
    if(iLevel >= mMinLogLevel) {
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iSender, iMsg, args);
        va_end(args);
    }
}

void IAppLogger::logCritical(const c8* sender, const c8* msg, ...) {
    if(ELOG_CRITICAL >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_CRITICAL, sender, msg, args);
        va_end(args);
    }
}
void IAppLogger::logCritical(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_CRITICAL >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_CRITICAL, sender, msg, args);
        va_end(args);
    }
}


void IAppLogger::logError(const c8* sender, const c8* msg, ...) {
    if(ELOG_ERROR >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_ERROR, sender, msg, args);
        va_end(args);
    }
}
void IAppLogger::logError(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_ERROR >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_ERROR, sender, msg, args);
        va_end(args);
    }
}


void IAppLogger::logWarning(const c8* sender, const c8* msg, ...) {
    if(ELOG_WARN >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_WARN, sender, msg, args);
        va_end(args);
    }
}
void IAppLogger::logWarning(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_WARN >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_WARN, sender, msg, args);
        va_end(args);
    }
}


void IAppLogger::logInfo(const c8* sender, const c8* msg, ...) {
    if(ELOG_INFO >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_INFO, sender, msg, args);
        va_end(args);
    }
}
void IAppLogger::logInfo(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_INFO >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_INFO, sender, msg, args);
        va_end(args);
    }
}


void IAppLogger::logDebug(const c8* sender, const c8* msg, ...) {
    if(ELOG_DEBUG >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_DEBUG, sender, msg, args);
        va_end(args);
    }
}
void IAppLogger::logDebug(const wchar_t* sender, const wchar_t* msg, ...) {
    if(ELOG_DEBUG >= mMinLogLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELOG_DEBUG, sender, msg, args);
        va_end(args);
    }
}


void IAppLogger::setLogLevel(const ELogLevel logLevel) {
    mMinLogLevel = logLevel;
}

void IAppLogger::postLog(ELogLevel level, const c8* sender, const c8* msg, va_list args) {
    if(sender && msg) {
        CAutoLock ak(mMutex);
        c8 tmstr[20];
        IAppTimer::getTimeAsString(tmstr, sizeof(tmstr));
        vsnprintf(mTextBuffer, MAX_TEXT_BUFFER_SIZE, msg, args);
        for(u32 it = 0; it < mAllReceiver.size(); ++it) {
            mAllReceiver[it]->log(level, tmstr, sender, mTextBuffer);
        }
    }
}

void IAppLogger::postLog(ELogLevel level, const wchar_t* sender, const wchar_t* msg, va_list args) {
    if(sender && msg) {
        CAutoLock ak(mMutex);
        wchar_t tmstr[20];
        IAppTimer::getTimeAsString(tmstr, sizeof(tmstr));
#if defined(APP_PLATFORM_WINDOWS)
        _vsnwprintf(mTextBufferW, MAX_TEXT_BUFFER_SIZE, msg, args);
        for(u32 it = 0; it < mAllReceiver.size(); ++it) {
            mAllReceiver[it]->log(level, tmstr, sender, mTextBufferW);
        }
#endif
    }
}


bool IAppLogger::add(IAntLogReceiver* pReceiver) {
    if(pReceiver) {
        CAutoLock ak(mMutex);
        mAllReceiver.push_back(pReceiver);
    }
    return true;
}


void IAppLogger::remove(const IAntLogReceiver* iLog) {
    CAutoLock ak(mMutex);
    for(u32 it = 0; it < mAllReceiver.size(); ++it) {
        if(iLog == mAllReceiver[it]) {
            delete mAllReceiver[it];
            mAllReceiver.erase(it);
            break;
        }
    }
}


void IAppLogger::clear() {
    CAutoLock ak(mMutex);
    for(u32 it = 0; it < mAllReceiver.size(); ++it) {
        delete mAllReceiver[it];
    }
    mAllReceiver.set_used(0);
}


}//namespace irr
