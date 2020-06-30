#pragma once
#include "HConfig.h"
#include "CSpinlock.h"

namespace app {
namespace core {

class CSpeedLimiter {
public:
    CSpeedLimiter();

    ~CSpeedLimiter();

    /**
    *@brief Get the sleep time required for the current session.
    *@return sleep time, in milliseconds. It's means overspeed if not zero.
    */
    u32 getLimitTime();

    void setFrequency(const s32 iFrequency) {
        mFrequency = ((iFrequency > 0 && iFrequency <= 0x0FFFFFFF)
            ? iFrequency : mFrequency);
    }

    s32 getFrequency() const {
        return mFrequency;
    }

    void setCurrentCount(const s32 iVal);

    s32 getCount()const {
        return mCount;
    }

private:
    CSpinlock mLock;
    s32 mFrequency;       ///<packets per second
    s32 mCount;           ///<packets count
    s64 mTime;            ///<last time
};

}//namespace core 
}//namespace app 
