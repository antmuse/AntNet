#ifndef APP_CNETHTTPHEAD_H
#define APP_CNETHTTPHEAD_H

#include "irrString.h"
#include "irrMap.h"
//#include "irrArray.h"

namespace irr {
namespace net {
class CNetPacket;


enum EHttpHeadID {
    EHHID_HOST,
    EHHID_LOCATION,
    EHHID_KEEP_ALIVE,
    EHRID_CONNECTION,
    EHRID_ACCEPT,
    EHHID_CONTENT_TYPE,
    EHHID_CONTENT_SIZE,
    EHHID_USER_AGENT,
    EHRID_ACCEPT_ENCODE,
    EHRID_REFERER,
    EHRID_ACCEPT_LANGUAGE,
    EHRID_ACCEPT_CHARSET,
    EHRID_AUTHORIZATION,
    EHRID_COOKIE,
    EHRID_DATE,
    EHRID_RANGE,
    EHRID_CONTENT_MD5,
    EHRID_CONTENT_RANGE,
    EHRID_CONTENT_LOCATION,
    EHRID_SERVER,
    EHRID_X_POWERED_BY,
    EHRID_TRANSFER_ENCODE,
    EHRID_COUNT
};


const c8* const AppHttpHeadIDName[] = {
    "Host",
    "Location",
    "Keep-Alive",
    "Connection",
    "Accpet",
    "Content-Type",
    "Content-Length",
    "User-Agent",
    "Accept-Encoding",
    "Referer",
    "Accept-Language",
    "Accept-Charset",
    "Authorization",
    "Cookie",
    "Date",
    "Range",
    "Content-MD5",
    "Content-Range",
    "Content-Location",
    "Server",
    "X-Powered-By",
    "Transfer-Encoding",
    0
};


const u8 AppHttpHeadIDNameSize[] = {
    (u8) ::strlen(AppHttpHeadIDName[EHHID_HOST]),
    (u8) ::strlen(AppHttpHeadIDName[EHHID_LOCATION]),
    (u8) ::strlen(AppHttpHeadIDName[EHHID_KEEP_ALIVE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_CONNECTION]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_ACCEPT]),
    (u8) ::strlen(AppHttpHeadIDName[EHHID_CONTENT_TYPE]),
    (u8) ::strlen(AppHttpHeadIDName[EHHID_CONTENT_SIZE]),
    (u8) ::strlen(AppHttpHeadIDName[EHHID_USER_AGENT]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_ACCEPT_ENCODE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_REFERER]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_ACCEPT_LANGUAGE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_ACCEPT_CHARSET]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_AUTHORIZATION]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_COOKIE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_DATE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_RANGE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_CONTENT_MD5]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_CONTENT_RANGE]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_CONTENT_LOCATION]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_SERVER]),
    (u8) ::strlen(AppHttpHeadIDName[EHRID_TRANSFER_ENCODE]),
    //(u8) ::strlen(AppHttpHeadIDName[EHRID_COUNT])
};


class CNetHttpHead {
public:

    CNetHttpHead();

    ~CNetHttpHead();

    const core::stringc* getValue(EHttpHeadID id)const {
        THttpHeadNode* it = mMapHead.find(id);
        return (it ? &it->getValue() : 0);
    }


    void setValue(EHttpHeadID id, const core::stringc& it) {
        mMapHead.set(id, it);
    }


    void removeHead(EHttpHeadID id) {
        mMapHead.remove(id);
    }


    void getBuffer(CNetPacket& out)const;


    void clear() {
        mMapHead.clear();
    }


private:
    typedef core::map<EHttpHeadID, core::stringc>::Node THttpHeadNode;
    typedef core::map<EHttpHeadID, core::stringc>::Iterator THttpHeadIterator;
    core::map<EHttpHeadID, core::stringc> mMapHead;
};


} //namespace net
} //namespace irr

#endif //APP_CNETHTTPHEAD_H
