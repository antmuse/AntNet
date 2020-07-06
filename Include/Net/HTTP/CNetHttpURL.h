#ifndef APP_CNETHTTPURL_H
#define APP_CNETHTTPURL_H

#include "CString.h"
#include "AppArray.h"
#include "http_parser.h"

namespace app {
namespace net {
/**
* @brief def a tool for URL
*   CNetHttpURL url("http://usr:passd@nginx.org:89/docs/guide.html?pa=1&pb=2#proxy");
*/
class CNetHttpURL {
public:
    CNetHttpURL();

    CNetHttpURL(const s8* url);
    CNetHttpURL(const s8* url, u64 len);
    CNetHttpURL(const core::CString& url);
    CNetHttpURL(const CNetHttpURL& other);

    ~CNetHttpURL();


    CNetHttpURL& operator=(const CNetHttpURL& other);

    bool operator==(const CNetHttpURL& other) const;

    bool operator!=(const CNetHttpURL& other) const;

    /**
    *@brief Set URL values.
    *@param url The URL string.
    *@return true if valid,  else false.
    */
    bool set(const core::CString& url);
    bool set(const s8* url, u64 len = 0);

    /**
    *@brief Set a path relatived to current host.
    *@param pth The relative path.
    */
    void setPath(const core::CString& pth) {
    }

    void getPath(core::CString& pth)const {
        if((1 << UF_PATH) & mNodes.field_set) {
            pth.setUsed(0);
            pth.append(mCache.constPointer() + mNodes.field_data[UF_PATH].off,
                mNodes.field_data[UF_PATH].len);
        } else {
            pth = "/";
        }
    }

    void getHost(core::CString& pth)const {
        if((1 << UF_HOST) & mNodes.field_set) {
            pth.setUsed(0);
            pth.append(mCache.constPointer() + mNodes.field_data[UF_HOST].off,
                mNodes.field_data[UF_HOST].len);
        }
    }

    void setHost(const core::CString& host) {
    }


    /**
    * @param idx @see http_parser_url_fields
    */
    bool getNode(const s32 idx, const s8*& str, s32& len) const {
        if((1 << idx) & mNodes.field_set) {
            str = mCache.constPointer() + mNodes.field_data[idx].off;
            len = mNodes.field_data[idx].len;
            return true;
        }
        str = nullptr;
        len = 0;
        return false;
    }

    u16 getPort() const {
        return mNodes.port;
    }

    bool isHTTP()const {
        const s8* str;
        s32 len;
        if(getNode(UF_SCHEMA, str, len) && 4 == len) {
            return *reinterpret_cast<s32*>("http") == *reinterpret_cast<const s32*>(str);
        }
        return false;
    }

    bool isHTTPS()const {
        const s8* str;
        s32 len;
        if(getNode(UF_SCHEMA, str, len) && 5 == len) {
            return *reinterpret_cast<s32*>("http") == *reinterpret_cast<const s32*>(str) && 's' == str[4];
        }
        return false;
    }

    u16 getMask()const {
        return mNodes.field_set;
    }


    void show();

protected:
    /**
    scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
    https://www.baidu.com/s?ie=utf-8&f=8&rsv_bp=1
    */
    core::TArray<s8> mCache;
    http_parser_url mNodes;

    void toLow(s32 idx);
};

} //namespace net
} //namespace app

#endif //APP_CNETHTTPURL_H
