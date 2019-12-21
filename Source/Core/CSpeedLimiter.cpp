#include "CSpeedLimiter.h"
#include "IAppTimer.h"
#include "HAtomicOperator.h"

namespace irr {
namespace core {


CSpeedLimiter::CSpeedLimiter() :
    mFrequency(0x0FFFFFFF),
    mCount(0),
    mTime(IAppTimer::getRelativeTime()) {
}


CSpeedLimiter::~CSpeedLimiter() {
}


void CSpeedLimiter::setCurrentCount(const s32 iVal) {
    AppAtomicFetchSet(iVal, &mCount);
}


u32 CSpeedLimiter::getLimitTime() {
    const s32 cnt = AppAtomicIncrementFetch(&mCount);
    if (cnt <= mFrequency) {
        return 0;
    }
    if (mLock.trylock()) {
        u32 ret = 0;
        const s64 curr = IAppTimer::getRelativeTime();
        const s64 sub = curr - mTime;
        if (sub >= 1000) {
            AppAtomicFetchSet(curr, &mTime);
            AppAtomicFetchSet(1, &mCount);
        } else {
            ret = static_cast<u32>(1000LL - sub);
        }
        mLock.unlock();
        return ret;
    }
    return 5; //sleep 5 ms
}


}//namespace core 
}//namespace irr 