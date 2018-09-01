﻿#include <time.h>
#include <stdarg.h>
#include "IUtility.h"
#include "CMutex.h"
#include "IAppLogger.h"
#include "CConsoleLogReceiver.h"
//#include "CFileLogReceiver.h"

namespace irr {

CMutex AppLogMutex;

c8 IAppLogger::mTextBuffer[MAX_TEXT_BUFFER_SIZE];
wchar_t IAppLogger::mTextBufferW[MAX_TEXT_BUFFER_SIZE];
core::list<IAntLogReceiver*> IAppLogger::mAllReceiver;

#ifdef   APP_DEBUG
ELogLevel IAppLogger::mMinLogLevel = ELOG_DEBUG;
#else       //for release version
ELogLevel IAppLogger::mMinLogLevel = ELOG_INFO;
#endif  //release version


IAppLogger::IAppLogger() {
    //IAppLogger::mMinLogLevel = ELOG_DEBUG;

#ifdef APP_COMPILE_WITH_CONSOLE_LOG_RECEIVER
    IAppLogger::add(new CConsoleLogReceiver());
#endif

#ifdef APP_COMPILE_WITH_FILE_LOG_RECEIVER
    IAppLogger::add(new CFileLogReceiver());
#endif
}


IAppLogger::~IAppLogger() {
}


IAppLogger* IAppLogger::getInstance() {
    static IAppLogger iLogger;
    return &iLogger;
}


void IAppLogger::log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMsg, ...) {
    if(iLevel >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iSender, iMsg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}
void IAppLogger::log(ELogLevel iLevel, const c8* iSender, const c8* iMsg, ...) {
    if(iLevel >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iSender, iMsg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}

void IAppLogger::logCritical(const c8* sender, const c8 *msg, ...) {
    if(ELOG_CRITICAL >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_CRITICAL, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}
void IAppLogger::logCritical(const wchar_t* sender, const wchar_t *msg, ...) {
    if(ELOG_CRITICAL >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_CRITICAL, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}


void IAppLogger::logError(const c8* sender, const c8 *msg, ...) {
    if(ELOG_ERROR >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_ERROR, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}
void IAppLogger::logError(const wchar_t* sender, const wchar_t *msg, ...) {
    if(ELOG_ERROR >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_ERROR, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}


void IAppLogger::logWarning(const c8* sender, const c8 *msg, ...) {
    if(ELOG_WARNING >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_WARNING, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}
void IAppLogger::logWarning(const wchar_t* sender, const wchar_t *msg, ...) {
    if(ELOG_WARNING >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_WARNING, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}


void IAppLogger::logInfo(const c8* sender, const c8 *msg, ...) {
    if(ELOG_INFO >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_INFO, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}
void IAppLogger::logInfo(const wchar_t* sender, const wchar_t *msg, ...) {
    if(ELOG_INFO >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_INFO, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}


void IAppLogger::logDebug(const c8* sender, const c8 *msg, ...) {
    if(ELOG_DEBUG >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_DEBUG, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}
void IAppLogger::logDebug(const wchar_t* sender, const wchar_t *msg, ...) {
    if(ELOG_DEBUG >= mMinLogLevel) {
        AppLogMutex.lock();
        va_list args;
        va_start(args, msg);
        postLog(ELOG_DEBUG, sender, msg, args);
        va_end(args);
        AppLogMutex.unlock();
    }
}


void IAppLogger::setLogLevel(const ELogLevel logLevel) {
    AppLogMutex.lock();
    mMinLogLevel = logLevel;
    AppLogMutex.unlock();
}

void IAppLogger::postLog(ELogLevel level, const c8* sender, const c8* msg, va_list args) {
    //f32 messageTime = (clock() - mStartTime) / (f32) CLOCKS_PER_SEC;
    vsnprintf(mTextBuffer, MAX_TEXT_BUFFER_SIZE, msg, args);
    for(CLogIterator it = mAllReceiver.begin(); it != mAllReceiver.end(); it++) {
        (*it)->log(level, sender, mTextBuffer/*, messageTime*/);
    }
}
void IAppLogger::postLog(ELogLevel level, const wchar_t* sender, const wchar_t* msg, va_list args) {
    //f32 messageTime = (clock() - mStartTime) / (f32)CLOCKS_PER_SEC;
#if defined(APP_PLATFORM_WINDOWS)
    _vsnwprintf(mTextBufferW, MAX_TEXT_BUFFER_SIZE, msg, args);
    for(CLogIterator it = mAllReceiver.begin(); it != mAllReceiver.end(); it++) {
        (*it)->log(level, sender, mTextBufferW/*, messageTime*/);
    }
#endif
}


bool IAppLogger::add(IAntLogReceiver* pReceiver) {
    AppLogMutex.lock();
    mAllReceiver.push_back(pReceiver);
    AppLogMutex.unlock();
    return true;
}

void IAppLogger::remove(const IAntLogReceiver* iLog) {
    AppLogMutex.lock();
    for(CLogIterator it = mAllReceiver.begin(); it != mAllReceiver.end(); it++) {
        if(iLog == (*it)) {
            delete (*it);
            mAllReceiver.erase(it);
            break;
        }
    }
    AppLogMutex.unlock();
}

//bool IAppLogger::isLogReceiverRegistered(const c8* name) {
//    AppLogMutex.lock();
//    core::stringc logName = safeString(name);
//    bool result = false;
//    for (CLogIterator it = mAllReceiver.begin(); it != mAllReceiver.end(); it++) {
//        if (logName == (*it)->getName()) {
//            result = true;
//            break;
//        }
//    }
//    AppLogMutex.unlock();
//    return result;
//}
//
//IAntLogReceiver* IAppLogger::getLogReceiver(const c8* name) {
//    AppLogMutex.lock();
//    core::stringc logName = safeString(name);
//    for (CLogIterator it = mAllReceiver.begin(); it != mAllReceiver.end(); it++) {
//        if (logName == (*it)->getName()) {
//            AppLogMutex.unlock();
//            return (*it);
//        }
//    }
//    AppLogMutex.unlock();
//    return NULL;
//}


}//namespace irr {
