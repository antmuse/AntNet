#ifndef APP_CNETHTTPREQUEST_H
#define APP_CNETHTTPREQUEST_H

#include "AppArray.h"
#include "AppMap.h"
#include "CNetHttpURL.h"
#include "CNetHttpHead.h"

namespace app {
namespace net {
class CNetPacket;

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


    const core::CString& getHost()const {
        return "";//        return mURL.getHost();
    }


    const core::CString& getPath()const {
        return "";//        return mURL.getPath();
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


    void getBuffer(CNetPacket& out)const;

    void clear();

private:
    core::CString mHttpVersion;
    core::CString mMethod;
    CNetHttpHead mHead;
    CNetHttpURL mURL;
    //core::TMap<core::CString, core::CString> mParameter;
};


} //namespace net
} //namespace app

#endif //APP_CNETHTTPREQUEST_H
