#include "HNetConfig.h"  
#include "HAtomicOperator.h"  
#include <stdio.h>  
#include <stdlib.h>  
#include <memory.h>

namespace irr {
namespace net {


CNetConfig::CNetConfig() {
    ::memset(this, 0, sizeof(CNetConfig));
    mReference = 1;
}

void CNetConfig::grab() {
    AppAtomicIncrementFetch(&mReference);
}

void CNetConfig::drop() {
    s32 ret = AppAtomicDecrementFetch(&mReference);
    APP_ASSERT(ret >= 0);
    if(0 == ret) {
        delete this;
    }
}

bool CNetConfig::check() {
    return true;
}

void CNetConfig::print() const {
    printf("CNetConfig::print>> mReuse=%s\n", mReuse ? "true" : "false");
    printf("CNetConfig::print>> mMaxPostAccept=%u\n", mMaxPostAccept);
    printf("CNetConfig::print>> mMaxFatchEvents=%u\n", mMaxFatchEvents);
    printf("CNetConfig::print>> mMaxContext=%u\n", mMaxContext);
    printf("CNetConfig::print>> mPollTimeout=%u ms\n", mPollTimeout);
}

}//namespace net
}//namespace irr
