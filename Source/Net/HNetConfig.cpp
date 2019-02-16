#include "HNetConfig.h"  
#include "HAtomicOperator.h"  
#include <stdio.h>  
#include <stdlib.h>  
#include <memory.h>
#include "irrMath.h"  

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
    mMaxWorkThread = core::clamp<u8>(mMaxWorkThread, 1, 255);
    mMaxPostAccept = core::clamp<u8>(mMaxPostAccept, 8, 255);
    mMaxFetchEvents = core::clamp<u16>(mMaxFetchEvents, 32, 255);
    mMaxContext = core::clamp<u32>(mMaxContext, 16, 0xFFFF);
    mPollTimeout = core::clamp<u32>(mPollTimeout, 10, 5 * 1000);
    mSessionTimeout = core::clamp<u32>(mSessionTimeout, mPollTimeout + 2 * 1000, 30 * 1000);
    return true;
}

void CNetConfig::print() const {
    printf("CNetConfig::print>> mReuse=%s\n", mReuse ? "true" : "false");
    printf("CNetConfig::print>> mMaxPostAccept=%u\n", mMaxPostAccept);
    printf("CNetConfig::print>> mMaxFetchEvents=%u\n", mMaxFetchEvents);
    printf("CNetConfig::print>> mMaxContext=%u\n", mMaxContext);
    printf("CNetConfig::print>> mPollTimeout=%u ms\n", mPollTimeout);
    printf("CNetConfig::print>> mSessionTimeout=%u ms\n", mSessionTimeout);
}

}//namespace net
}//namespace irr
