#ifndef APP_CNETMAILMESSAGE_H
#define APP_CNETMAILMESSAGE_H

#include "CString.h"
//#include "AppMap.h"
//#include "AppArray.h"

namespace app {
namespace net {
//class CNetPacket;

class CNetMailMessage {
public:

    CNetMailMessage();

    ~CNetMailMessage();

    void setTitle(const core::CString& it) {
        mTitle = it;
    }

    void setFrom(const core::CString& it) {
        mMailFrom = it;
    }

    void setTo(const core::CString& it) {
        mMailTo = it;
    }

    void clear() {
    }

private:
    core::CString mTitle;
    core::CString mMailFrom;
    core::CString mMailTo;
};


} //namespace net
} //namespace app

#endif //APP_CNETMAILMESSAGE_H
