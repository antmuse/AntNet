#include "HNetConfig.h"  
#include "HAtomicOperator.h"  
#include <stdio.h>  

namespace irr {
namespace net {

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

void CNetConfig::print() const {
    printf("CNetConfig::print>> mReuse=%s\n", mReuse ? "true" : "false");
    printf("CNetConfig::print>> mMaxPostAccept=%u\n", mMaxPostAccept);
    printf("CNetConfig::print>> mMaxFatchEvents=%u\n", mMaxFatchEvents);
    printf("CNetConfig::print>> mMaxContext=%u\n", mMaxContext);
    printf("CNetConfig::print>> mPollTimeout=%u ms\n", mPollTimeout);
}

}//namespace net
}//namespace irr