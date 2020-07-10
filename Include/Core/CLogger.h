/**
 * @file   CLogger.h
 * @brief    日志记录
 *
 *  Copyright (c) 2010-2011 Antmuse
 *  This file is part of my "Legend" task.
 *  For conditions of distribution and use, see copyright notice in HConfig.h
 * @author   Antmuse|Email:antmuse@live.cn
 * @version   1.0.0
 * @date  2010年11月26日, 下午10:16
 */
#ifndef APP_CLOGGER_H
#define	APP_CLOGGER_H

#include "HConfig.h"
#include <stdarg.h>
#include "AppArray.h"
#include "HEngineLog.h"
#include "CMutex.h"


#if defined(APP_DEBUG) && defined(APP_PLATFORM_WINDOWS)
#define APP_LOG(TYPE, FROM, FORMAT, ...)  CLogger::log(TYPE, FROM, FORMAT, __VA_ARGS__)
#else
#define APP_LOG(TYPE, FROM, FORMAT, ...)
#endif

namespace app {
class ILogReceiver;

/**
 * @class   CLogger
 * @brief    日志记录器接口
 *
 * 用于记录不同类别的日志。
 * 运行日志可记录到文件中，也可以记录到cmd窗口。
 * @author   Antmuse|Email:antmuse@live.cn
 * @version   1.0.0
 * @date  2010年11月26日, 下午10:16
 */
class CLogger {
public:
    enum ELoggerRecieverType {
        ELRT_CONSOLE = 1,
        ELRT_FILE_TEXT = 2,
        ELRT_FILE_HTML = 4,
        ELRT_MAX = 8
    };


    static CLogger& getInstance();

    static void addReceiver(u32 flag);

    static void log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMsg, ...);

    static void log(ELogLevel iLevel, const s8* iSender, const s8* iMsg, ...);

    //! Used to log a critical error message to the logging system.
    /**
    \param sender: Name of the class/component sending the message.
    \param msg: The message to send. */
    static void logCritical(const wchar_t* sender, const wchar_t *msg, ...);
    static void logCritical(const s8* sender, const s8 *msg, ...);

    //! Used to log an error message to the logging system.
    /**
    \param sender: Name of the class/component sending the message.
    \param msg: The message to send. */
    static void logError(const wchar_t* sender, const wchar_t *msg, ...);
    static void logError(const s8* sender, const s8 *msg, ...);

    //! Used to log a warning to the logging system.
    /**
    \param sender: Name of the class/component sending the message.
    \param msg: The message to send. */
    static void logWarning(const wchar_t* sender, const wchar_t *msg, ...);
    static void logWarning(const s8* sender, const s8 *msg, ...);

    //! Used to log an informational message to the logging system.
    /**
    \param sender: Name of the class/component sending the message.
    \param msg: The message to send. */
    static void logInfo(const wchar_t* sender, const wchar_t *msg, ...);
    static void logInfo(const s8* sender, const s8 *msg, ...);

    //! Used to log a debug message to the logging system.
    /**
    \param sender: Name of the class/component sending the message.
    \param msg: The message to send. */
    static void logDebug(const wchar_t* sender, const wchar_t *msg, ...);
    static void logDebug(const s8* sender, const s8 *msg, ...);

    //! Returns the minimum log level that will be sent to the log receivers.
    static const ELogLevel getLogLevel(){
        return mMinLogLevel;
    }

    //! Sets the minimum log level that the engine will send to log receivers.
    /** Primarily used to prevent too much verbose information from being sent to disk/console.
    \param logLevel: ELogLevel to set as the new minimum. Anything equal to or greater than this level will be logged. */
    static void setLogLevel(const ELogLevel logLevel);

    //! Register Log Receiver.
    /** Note: Any class registered will become owned by the internal thread.
    If threading is enabled, you MUST make the receiver threadsafe if you plan to access it in your application while it is registered.
    \param iLog: The log receiver.
    \return True on success, False on failure. */
    static bool add(ILogReceiver* iLog);

    //! Unregister a Log Receiver.
    /** Will NOT delete any user added receiver, you must do that yourself.
    \param iLog: The log receiver to remove. */
    static void remove(const ILogReceiver* iLog);

    /**
    * @brief remove all log receivers.
    */
    static void clear();


protected:
    static void postLog(ELogLevel level, const s8* sender, const s8* msg, va_list args);
    static void postLog(ELogLevel level, const wchar_t* sender, const wchar_t* msg, va_list args);
    const static u16 MAX_TEXT_BUFFER_SIZE = 1024;
    static s8 mTextBuffer[MAX_TEXT_BUFFER_SIZE];
    static wchar_t mTextBufferW[MAX_TEXT_BUFFER_SIZE];
    static ELogLevel mMinLogLevel;
    static CMutex mMutex;
    static core::TArray<ILogReceiver*> mAllReceiver;

private:
    //ILogger* pIrrLogger;
    CLogger();
    CLogger(CLogger& it) = delete;
    CLogger& operator=(CLogger& it) = delete;
    virtual ~CLogger();
};

//APP_API CLogger* getLogger();
} //namespace app

#endif	/* APP_CLOGGER_H */

