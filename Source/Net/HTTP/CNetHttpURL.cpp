#include "CNetHttpURL.h"
#include "IUtility.h"
#include "fast_atof.h"


namespace irr {
namespace net {

CNetHttpURL::CNetHttpURL() :
    mPath("/"),
    mHTTPS(false),
    mPort(APP_NET_DEFAULT_HTTP_PORT) {
}


CNetHttpURL::CNetHttpURL(const core::stringc& url) :
    //mPath("/"),
    mHTTPS(false),
    mPort(APP_NET_DEFAULT_HTTP_PORT) {
    set(url);
}


CNetHttpURL::CNetHttpURL(const CNetHttpURL& other) {
    *this = other;
}


CNetHttpURL::~CNetHttpURL() {
}


CNetHttpURL& CNetHttpURL::operator=(const CNetHttpURL& other) {
    if(this == &other) {
        return *this;
    }
    mHTTPS = other.mHTTPS;
    mPort = other.mPort;
    mHost = other.mHost;
    mPath = other.mPath;

    return *this;
}


bool CNetHttpURL::operator==(const CNetHttpURL& other) const {
    if(this == &other) {
        return true;
    }
    if(mHTTPS == other.mHTTPS &&
        mPort == other.mPort &&
        mHost == other.mHost &&
        mPath == other.mPath) {
        return true;
    }
    return false;
}


bool CNetHttpURL::operator!=(const CNetHttpURL& other) const {
    if(this == &other) {
        return false;
    }
    if(mHTTPS != other.mHTTPS ||
        mPort != other.mPort ||
        mHost != other.mHost ||
        mPath != other.mPath) {
        return true;
    }
    return false;
}


void CNetHttpURL::mergePath(const core::stringc& it) {
    if('/' == it[0]) {
        mPath = it;
    } else {
        s32 pos = mPath.findLast('/');
        if(pos >= 0) {
            mPath = mPath.subString(0, 1 + pos);
        } else {
            mPath = "/";
        }
        mPath += it;
    }
}


bool CNetHttpURL::set(const core::stringc& it) {
    core::stringc url(it);
    s32 pos = url.findLast('#');
    if(pos > 0) {
        url = url.subString(0, pos);
    } else if(0 == pos) {
        return false;
    }

    const u32 min = 8;
    if(url.size() <= min) {
        mergePath(url);
        return true;
    }

    bool full_url = false;
    if(':' == url[4] && '/' == url[5] && '/' == url[6]) {//http://
        core::AppToLower(&url[0], 4);
        full_url = true;
    } else if(':' == url[5] && '/' == url[6] && '/' == url[7]) {//https://
        core::AppToLower(&url[0], 5);
        full_url = true;
    }

    if(full_url && 'h' == url[0] && 't' == url[1] && 't' == url[2] && 'p' == url[3]) {
        mHTTPS = ('s' == url[4] ? true : false);
    } else {
        mergePath(url);
        return true;
    }

    s32 start = (mHTTPS ? 8 : 7); //// skip "http://" or "https://"
    s32 end = url.findNext('/', start);
    if(end == start) {
        return false;
    }if(end > start) {
        mHost = url.subString(start, end - start);
        mPath = url.subString(end, url.size() - end);
    } else {
        mHost = url.subString(start, url.size() - start);
        mPath = "/";
    }

    end = mHost.findLast(':');
    if(end > 0) {
        mPort = core::strtoul10(mHost.c_str() + end + 1);
        mHost = mHost.subString(0, end);
    } else {
        mPort = (mHTTPS ? APP_NET_DEFAULT_HTTPS_PORT : APP_NET_DEFAULT_HTTP_PORT);
    }

    mHost.make_lower();
    return true;
}


u32 CNetHttpURL::setAndCheck(const core::stringc& url) {
    u32 ret = 0;
    const core::stringc opath(mPath);
    const core::stringc ohost(mHost);
    const u16 oport = mPort;
    const bool ohttps = mHTTPS;
    if(!set(url)) {
        return ret;
    }
    if(opath != mPath) {
        ret |= EURL_NEW_PATH;
    }
    if(!isSameHost(ohost, mHost)) {
        ret |= EURL_NEW_HOST;
    }
    if(oport != mPort) {
        ret |= EURL_NEW_PORT;
    }
    if(ohttps != mHTTPS) {
        ret |= EURL_NEW_HTTP;
    }
    return ret;
}


bool CNetHttpURL::isSameHost(const core::stringc& host1, const core::stringc& host2) {
    if(host1 == host2) {
        return true;
    }
    s32 pos1 = 0;
    s32 pos2 = 0;
    s32 sz1 = host1.size();
    s32 sz2 = host2.size();
    s32 extsz1, extsz2;
    s32 count = 0;
    for(; sz1 > 0 && sz2 > 0;) {
        pos1 = host1.findLast('.', pos1 - 1);
        pos2 = host2.findLast('.', pos2 - 1);
        extsz1 = sz1 - pos1 - 1;
        extsz2 = sz2 - pos2 - 1;
        sz1 = pos1;
        sz2 = pos2;
        if(extsz1 != extsz2) {
            return false;
        }
        if(0 != memcmp(host1.c_str() + pos1 + 1, host2.c_str() + pos2 + 1, extsz1)) {
            return false;
        }
        if(0 == count) {
            ++count;
            continue;
        }
        if(isPublicExt(host1.c_str() + pos1 + 1, extsz1)) {
            continue;
        }
        return true;
    }
    return false;
}


bool CNetHttpURL::isPublicExt(const c8* host, s32 size) {
    const c8* const ext2[] = {
        "cn",
        "cc",
        0
    };
    const c8* const ext3[] = {
        "com",
        "net",
        "gov",
        "org",
        0
    };
    switch(size) {
    case 3:
        for(u32 i = 0; ext3[i]; ++i) {
            if(0 == memcmp(ext3[i], host, size)) {
                return true;
            }
        }
        break;
    case 2:
        for(u32 i = 0; ext2[i]; ++i) {
            if(0 == memcmp(ext2[i], host, size)) {
                return true;
            }
        }
        break;
    default: break;
    }

    return false;
}



bool CNetHttpURL::isValidChar(c8 c) {
    if((c >= 'A'&&c <= 'Z') || (c >= 'a'&&c <= 'z') || (c >= '0'&&c <= '9')) {
        return true;
    }

    c8 url[17] = {
        '.', '_', '\\', '/', '~',
        '%', '-', '+', '&', '#',
        '?', '!', '=', '(', ')',
        '@', ':'
    };

    for(s32 i = 0; i < 17; i++) {
        if(c == url[i]) {
            return true;
        }
    }

    return false;
}


} //namespace net
} //namespace irr