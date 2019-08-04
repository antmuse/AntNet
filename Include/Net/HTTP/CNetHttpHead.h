#ifndef APP_CNETHTTPHEAD_H
#define APP_CNETHTTPHEAD_H

#include "irrString.h"
#include "irrMap.h"


#define APP_INIT_STRING(X) sizeof(X) - 1, X
#define APP_NULL_STRING    0, NULL


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

struct SHttpHead {
    u16 mID;
    u8 mLen;
    c8* mKey;
};

const SHttpHead AppHttpHeads[] = {
    {EHHID_HOST, APP_INIT_STRING("Host")},
    {EHHID_LOCATION, APP_INIT_STRING("Location")},
    {EHHID_KEEP_ALIVE, APP_INIT_STRING("Keep-Alive")},
    {EHRID_CONNECTION, APP_INIT_STRING("Connection")},
    {EHRID_ACCEPT, APP_INIT_STRING("Accpet")},
    {EHHID_CONTENT_TYPE, APP_INIT_STRING("Content-Type")},
    {EHHID_CONTENT_SIZE, APP_INIT_STRING("Content-Length")},
    {EHHID_USER_AGENT, APP_INIT_STRING("User-Agent")},
    {EHRID_ACCEPT_ENCODE, APP_INIT_STRING("Accept-Encoding")},
    {EHRID_REFERER, APP_INIT_STRING("Referer")},
    {EHRID_ACCEPT_LANGUAGE, APP_INIT_STRING("Accept-Language")},
    {EHRID_ACCEPT_CHARSET, APP_INIT_STRING("Accept-Charset")},
    {EHRID_AUTHORIZATION, APP_INIT_STRING("Authorization")},
    {EHRID_COOKIE, APP_INIT_STRING("Cookie")},
    {EHRID_DATE, APP_INIT_STRING("Date")},
    {EHRID_RANGE, APP_INIT_STRING("Range")},
    {EHRID_CONTENT_MD5, APP_INIT_STRING("Content-MD5")},
    {EHRID_CONTENT_RANGE, APP_INIT_STRING("Content-Range")},
    {EHRID_CONTENT_LOCATION, APP_INIT_STRING("Content-Location")},
    {EHRID_SERVER, APP_INIT_STRING("Server")},
    {EHRID_X_POWERED_BY, APP_INIT_STRING("X-Powered-By")},
    {EHRID_TRANSFER_ENCODE, APP_INIT_STRING("Transfer-Encoding")}
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
