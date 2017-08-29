#ifndef APP_IAPPLOGGER_H
#define APP_IAPPLOGGER_H

#include "HConfig.h"
#include "irrTypes.h"

#if defined(APP_DEBUG)
#define APP_LOG(TYPE, FROM, FORMAT, ...)  (IAppLogger::log(TYPE, FROM, FORMAT,__VA_ARGS__))
#else
#define APP_LOG(TYPE, FROM, FORMAT, ...)
#endif


namespace irr {

	/// Enum of all supported log levels.
	enum ELogLevel {
		ELOG_DEBUG,
		ELOG_INFO,
		ELOG_WARNING,
		ELOG_ERROR,
		ELOG_CRITICAL,
		ELOG_COUNT
	};

	/// Contains strings for each log level to make them easier to print to a stream.
	const c8* const AppLogLevelStrings[] = {
		"Debug",
		"Information",
		"Warning",
		"Error",
		"Critical",
		0
	};

	const wchar_t* const AppLogLevelStringsW[] = {
		L"Debug",
		L"Information",
		L"Warning",
		L"Error",
		L"Critical",
		0
	};

	class IAppLogger {
	public:
		static void log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iFormat, ...);
		static void log(ELogLevel iLevel, const c8* iSender, const c8* iFormat, ...);
		static void setLevel(ELogLevel iLevel);

	private:
		IAppLogger();
		~IAppLogger();
		const static u16 MAX_TEXT_BUFFER_SIZE = 1024;
		static c8 mTextBuffer[MAX_TEXT_BUFFER_SIZE];
		static wchar_t mTextBufferW[MAX_TEXT_BUFFER_SIZE];
		static ELogLevel mMinLogLevel;
	};

} //namespace irr

#endif	/* APP_IAPPLOGGER_H */

