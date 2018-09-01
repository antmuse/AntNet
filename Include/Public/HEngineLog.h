﻿#ifndef ANTMUSE_HENGINELOG_H
#define	ANTMUSE_HENGINELOG_H

#include "irrTypes.h"

namespace irr {

/// Enum of all supported log levels in Legend.
enum ELogLevel {
	ELOG_DEBUG,
	ELOG_INFO,
	ELOG_WARNING,
	ELOG_ERROR,
	ELOG_CRITICAL,
	ELOG_COUNT
};

/// Contains strings for each log level to make them easier to print to a stream.
const c8* const LogLevelStrings[] = {
	"Debug",
	"Information",
	"Warning",
	"Error",
	"Critical",
	0
};

} //namespace irr

#endif	/* ANTMUSE_HENGINELOG_H */
