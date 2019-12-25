#ifndef ANTMUSE_ILOGRECEIVER_H
#define	ANTMUSE_ILOGRECEIVER_H

#include "HConfig.h"
#include "HEngineLog.h"

namespace irr {

/// Interface for receiving log messages and relaying them to some kind of output device or stream.
class IAntLogReceiver {
public:
    IAntLogReceiver() {
    }
    virtual ~IAntLogReceiver() {
    }
    /**
    * @brief Called on every logged message that is greater than or equal to the minimum log level.
    * @param sender: The class/component sending the message.
    * @param message: The log message itself.
    * @param level: Log level of the message.
    * @param time: Time in seconds that the message was sent.  This time has millisecond accuracy. */
    virtual bool log(ELogLevel level, const c8* timestr, const c8* sender, const c8* message) = 0;
    virtual bool log(ELogLevel level, const wchar_t* timestr, const wchar_t* sender, const wchar_t* message) = 0;
};

} //namespace irr

#endif	/* ANTMUSE_ILOGRECEIVER_H */

