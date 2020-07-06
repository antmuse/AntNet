#ifndef APP_CNETHTTPREQUEST_H
#define APP_CNETHTTPREQUEST_H

#include "AppArray.h"
#include "CNetHttpHead.h"
#include "CNetHttpURL.h"

namespace app {
namespace net {

class CNetHttpRequest {
public:

    CNetHttpRequest();


    ~CNetHttpRequest();


    CNetHttpURL& getURL() {
        return mURL;
    }


    CNetHttpHead& getHead() {
        return mHead;
    }

    const core::CString& getMethod()const {
        return mMethod;
    }


    void setMethod(const core::CString& it) {
        mMethod = it;
        mMethod.makeUpper();
    }


    const core::CString& getVersion()const {
        return mHttpVersion;
    }


    void setVersion(const core::CString& it) {
        mHttpVersion = it;
    }

    void setKeepAlive(bool val) {
        mHead.setValue("Connection", val ? "Keep-Alive" : "close");
    }

    void getBuffer(CNetPacket& out)const;

    void clear();

private:
    core::CString mHttpVersion;
    core::CString mMethod;
    CNetHttpHead mHead;
    CNetHttpURL mURL;
    //u32 mKeepAlive : 1;
};


} //namespace net
} //namespace app

#endif //APP_CNETHTTPREQUEST_H
