#ifndef APP_CNETHTTPREQUEST_H
#define APP_CNETHTTPREQUEST_H

#include "irrArray.h"
#include "irrMap.h"
#include "CNetHttpURL.h"
#include "CNetHttpHead.h"

namespace irr {
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


    const core::stringc& getHost()const {
        return mURL.getHost();
    }


    const core::stringc& getPath()const {
        return mURL.getPath();
    }


    const core::stringc& getMethod()const {
        return mMethod;
    }


    void setMethod(const core::stringc& it) {
        mMethod = it;
        mMethod.make_upper();
    }


    const core::stringc& getVersion()const {
        return mHttpVersion;
    }


    void setVersion(const core::stringc& it) {
        mHttpVersion = it;
    }


    void getBuffer(CNetPacket& out)const;

    void clear();

private:
    core::stringc mHttpVersion;
    core::stringc mMethod;
    CNetHttpHead mHead;
    CNetHttpURL mURL;
    //core::map<core::stringc, core::stringc> mParameter;
};


} //namespace net
} //namespace irr

#endif //APP_CNETHTTPREQUEST_H
