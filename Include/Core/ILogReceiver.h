#ifndef ANTMUSE_ILOGRECEIVER_H
#define	ANTMUSE_ILOGRECEIVER_H

#include "HConfig.h"
#include "HEngineLog.h"

namespace app {

/// Interface for receiving log messages and relaying them to some kind of output device or stream.
class ILogReceiver {
public:
    ILogReceiver() {
    }
    virtual ~ILogReceiver() {
    }
    /**
    * @brief Called on every logged message that is greater than or equal to the minimum log level.
    * @param sender: The class/component sending the message.
    * @param message: The log message itself.
    * @param level: Log level of the message.
    * @param time: Time in seconds that the message was sent.  This time has millisecond accuracy. */
    virtual bool log(ELogLevel level, const s8* timestr, const s8* sender, const s8* message) = 0;
    virtual bool log(ELogLevel level, const wchar_t* timestr, const wchar_t* sender, const wchar_t* message) = 0;
};

} //namespace app

#endif	/* ANTMUSE_ILOGRECEIVER_H */

