#ifndef APP_CNETHTTPURL_H
#define APP_CNETHTTPURL_H

#include "irrString.h"
#include "irrMap.h"

namespace irr {
namespace net {


#define APP_NET_DEFAULT_HTTP_PORT       80
#define APP_NET_DEFAULT_HTTPS_PORT      443


class CNetHttpURL {
public:
    enum EURLFlag {
        EURL_INVALID = 0,
        EURL_NEW_HTTP = 1,
        EURL_NEW_PATH = 2,
        EURL_NEW_HOST = 4,
        EURL_NEW_PORT = 8
    };
    CNetHttpURL();

    CNetHttpURL(const core::stringc& url);

    ~CNetHttpURL();

    CNetHttpURL(const CNetHttpURL& other);

    CNetHttpURL& operator=(const CNetHttpURL& other);

    bool operator==(const CNetHttpURL& other) const;

    bool operator!=(const CNetHttpURL& other) const;

    static bool isValidChar(c8 ch);

    /**
    *@brief Set URL values.
    *@param url The URL string.
    *@return Thre flags of changed parts of url.
    */
    u32 setAndCheck(const core::stringc& url);

    /**
    *@brief Set URL values.
    *@param url The URL string.
    *@return true if valid,  else false.
    */
    bool set(const core::stringc& url);

    /**
    *@brief Set a path relatived to current host.
    *@param pth The relative path.
    */
    void setPath(const core::stringc& pth) {
        mPath = pth;
    }

    void setHost(const core::stringc& host) {
        mHost = host;
    }

    u16 getPort() const {
        return mPort;
    }

    bool isHTTPS() const {
        return mHTTPS;
    }

    const core::stringc& getHost()const {
        return mHost;
    }

    const core::stringc& getPath()const {
        return mPath;
    }

    static bool isSameHost(const core::stringc& host1, const core::stringc& host2);

    static bool isPublicExt(const c8* host, s32 size);

protected:
    void mergePath(const core::stringc& it);
    bool mHTTPS; ///<default = false
    u16 mPort;
    core::stringc mHost;
    core::stringc mPath;
    //core::stringc mFile;
    //core::map<core::stringc, core::stringc> mParameter;
    //core::stringc mAnchor;
};

} //namespace net
} //namespace irr

#endif //APP_CNETHTTPURL_H
