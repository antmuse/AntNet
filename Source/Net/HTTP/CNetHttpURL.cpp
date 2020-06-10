#include "CNetHttpURL.h"
//#include "IUtility.h"
//#include "fast_atof.h"

namespace irr {
namespace net {
/*
static c8 lowcase[] =
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0" "0123456789\0\0\0\0\0\0"
"\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
"\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
*/


CNetHttpURL::CNetHttpURL() {
    http_parser_url_init(&mNodes);
}


CNetHttpURL::CNetHttpURL(const core::stringc& url) {
    set(url);
}

CNetHttpURL::CNetHttpURL(const c8* url) {
    set(url, 0);
}

CNetHttpURL::CNetHttpURL(const c8* url, u64 len) {
    set(url, len);
}


CNetHttpURL::CNetHttpURL(const CNetHttpURL& other) {
    *this = other;
}


CNetHttpURL::~CNetHttpURL() {
}

CNetHttpURL& CNetHttpURL::operator=(const CNetHttpURL& other) {
    if (this == &other) {
        return *this;
    }
    mCache.set_used(other.mCache.size());
    memcpy(mCache.pointer(), other.mCache.const_pointer(), other.mCache.size());
    mNodes = other.mNodes;
    return *this;
}

bool CNetHttpURL::operator==(const CNetHttpURL& other) const {
    if (this == &other) {
        return true;
    }
    if (mCache.size() != other.mCache.size()) {
        return false;
    }
    return 0 == memcmp(mCache.const_pointer(), other.mCache.const_pointer(), mCache.size());
}

bool CNetHttpURL::operator!=(const CNetHttpURL& other) const {
    return !(*this == other);
}

bool CNetHttpURL::set(const core::stringc& it) {
    return set(it.c_str(), it.size());
}

void CNetHttpURL::toLow(s32 idx) {
    c8* str;
    s32 len;
    if (getNode(idx, str, len)) {
        for (; --len >= 0;) {
            if (str[len] >= 'A' && str[len] <= 'Z') {
                str[len] += 32;
            }
        }
    }
}

bool CNetHttpURL::set(const c8* url, u64 len) {
    if (nullptr != url) {
        len = len > 0 ? len : strlen(url);
        mCache.set_used(len + 1);
        memcpy(mCache.pointer(), url, len + 1);
        http_parser_url_init(&mNodes);
        bool ret = 0 == http_parser_parse_url(url, len, 0, &mNodes);
        if (ret) {
            if (0 == mNodes.port) {
                mNodes.port = 80;
            }
            toLow(UF_SCHEMA);
            toLow(UF_HOST);
        }
        return ret;
    }
    return false;
}

void CNetHttpURL::show() {
    c8* str;
    s32 len;
    printf("[port=%d][bits=%X]\n", mNodes.port, mNodes.field_set);
    for (s32 i = 0; i < net::UF_MAX; ++i) {
        if (getNode(i, str, len)) {
            printf("item[%d][len=%u]=%.*s\n", i, len, len, str);
        }
    }
}

} //namespace net
} //namespace irr