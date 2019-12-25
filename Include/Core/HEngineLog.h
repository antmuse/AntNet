#ifndef ANTMUSE_HENGINELOG_H
#define	ANTMUSE_HENGINELOG_H

#include "irrTypes.h"

namespace irr {

/// Enum of all supported log levels in Legend.
enum ELogLevel {
    ELOG_DEBUG,
    ELOG_INFO,
    ELOG_WARN,
    ELOG_ERROR,
    ELOG_CRITICAL,
    ELOG_COUNT
};

/// Contains strings for each log level to make them easier to print to a stream.
const c8* const AppLogLevelNames[] = {
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Critical",
    0
};

const wchar_t* const AppWLogLevelNames[] = {
    L"Debug",
    L"Info",
    L"Warn",
    L"Error",
    L"Critical",
    0
};


} //namespace irr

#endif	/* ANTMUSE_HENGINELOG_H */

