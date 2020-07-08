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

    CNetHttpRequest(const CNetHttpRequest& val) :
        mHead(val.mHead),
        mURL(val.mURL),
        mHttpVersion(val.mHttpVersion),
        mMethod(val.mMethod) {
    }

    ~CNetHttpRequest();

    CNetHttpRequest& operator=(const CNetHttpRequest& val) {
        if(this != &val) {
            mHead = val.mHead;
            mURL = val.mURL;
            mHttpVersion = val.mHttpVersion;
            mMethod = val.mMethod;
        }
        return *this;
    }

    CNetHttpURL& getURL() {
        return mURL;
    }

    void setHead(const CNetHttpHead& val) {
        mHead = val;
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
};


} //namespace net
} //namespace app

#endif //APP_CNETHTTPREQUEST_H
