/**
* @brief TODO>>recode all
*/

#ifndef APP_CNETHTTPRESPONSE_H
#define APP_CNETHTTPRESPONSE_H

#include "AppMap.h"
#include "CString.h"
#include "HNetHttpStatus.h"
#include "CNetHttpHead.h"
#include "CNetPacket.h"

namespace app {
namespace net {


class CNetHttpResponse {
public:

    CNetHttpResponse();

    ~CNetHttpResponse();

    CNetHttpHead& getHead() {
        return mHead;
    }

    void setHead(const CNetHttpHead& val) {
        mHead = val;
    }

    const CNetHttpHead& getHead() const {
        return mHead;
    }

    u32 getPageSize()const {
        return mWebPage.getSize();
    }

    CNetPacket& getPage() {
        return mWebPage;
    }

    s32 getStatusCode() const {
        return mStatusCode;
    }

    void clear();

    //for parser
    void setCurrentHeader(const s8* val, u32 len) {
        mHttpHeader.setUsed(0);
        mHttpHeader.append(val, len);
    }

    //for parser
    void setCurrentValue(const s8* val, u32 len) {
        core::CString tmp(val, len);
        if((1 & mLowFlag) > 0) {
            mHttpHeader.makeLower();
        }
        if((2 & mLowFlag) > 0) {
            tmp.makeLower();
        }
        mHead.setValue(mHttpHeader, tmp);
    }

    //for parser
    void setStatus(s32 val) {
        mStatusCode = val;
    }

    //for parser
    void appendBuf(const s8* val, u32 len) {
        mWebPage.addBuffer(val, len);
    }

    //for parser
    void setBuf(const s8* val, u32 len) {
        mWebPage.setUsed(0);
        mWebPage.addBuffer(val, len);
    }

    void show(bool head = true);

    //for parser
    void setFinished(bool val) {
        mFinished = val;
    }

    bool getFinished() const {
        return mFinished;
    }

    /**
     * @brief for parser
     * @param flag 1=header field, 2=header value, 3=all
     */
    void setLowFlag(s32 flag) {
        mLowFlag = flag;
    }


private:
    bool mFinished;
    u8 mLowFlag;
    s32 mStatusCode;
    core::CString mHttpHeader;
    CNetHttpHead mHead;
    CNetPacket mWebPage;
};


} //namespace net
} //namespace app

#endif //APP_CNETHTTPRESPONSE_H
