#include "CNetHttpResponse.h"
#include "fast_atof.h"
#include "AppMath.h"
#include "IUtility.h"

namespace app {
namespace net {

CNetHttpResponse::CNetHttpResponse() :
    mStatusCode(0),
    mLowFlag(1),
    mFinished(false),
    mWebPage(1024) {
}


CNetHttpResponse::~CNetHttpResponse() {
}


void CNetHttpResponse::clear() {
    mStatusCode = EHS_INVALID_CODE;
    mHead.clear();
    mWebPage.shrink(1024);
    mFinished = false;
}


void CNetHttpResponse::show(bool head) {
    if(head) {
        printf("HTTP Status=%d\n", mStatusCode);
        mHead.show();
    }
    u32 bsz = mWebPage.getSize();
    s8* buf = mWebPage.getPointer();
    u32 curr = 1024;
    for(; curr < bsz; curr += 1024) {
        printf("%.*s", 1024, buf);
        buf += 1024;
    }
    curr = bsz - (curr - 1024);
    if(curr > 0) {
        printf("%.*s", curr, buf);
    }
}

} //namespace net
} //namespace app