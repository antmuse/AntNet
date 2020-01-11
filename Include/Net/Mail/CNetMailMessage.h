#ifndef APP_CNETMAILMESSAGE_H
#define APP_CNETMAILMESSAGE_H

#include "irrString.h"
//#include "irrMap.h"
//#include "irrArray.h"

namespace irr {
namespace net {
//class CNetPacket;

class CNetMailMessage {
public:

    CNetMailMessage();

    ~CNetMailMessage();

    void setTitle(const core::stringc& it) {
        mTitle = it;
    }

    void setFrom(const core::stringc& it) {
        mMailFrom = it;
    }

    void setTo(const core::stringc& it) {
        mMailTo = it;
    }

    void clear() {
    }

private:
    core::stringc mTitle;
    core::stringc mMailFrom;
    core::stringc mMailTo;
};


} //namespace net
} //namespace irr

#endif //APP_CNETMAILMESSAGE_H
