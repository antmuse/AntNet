#ifndef APP_CTIMERWHEEL_H
#define APP_CTIMERWHEEL_H

#include "CQueueNode.h"
#include "CSpinlock.h"

namespace irr {

//32bit = 8+6*4;  slot0=8bits, slot1 & slot2 & slot3 & slot4 = 6bits.
#define APP_TIME_SLOT_BITS		    6
#define APP_TIME_ROOT_SLOT_BITS		8
#define APP_TIME_SLOT_SIZE		    (1 << APP_TIME_SLOT_BITS)
#define APP_TIME_ROOT_SLOT_SIZE		(1 << APP_TIME_ROOT_SLOT_BITS)
#define APP_TIME_SLOT_MASK		    (APP_TIME_SLOT_SIZE - 1)
#define APP_TIME_ROOT_SLOT_MASK		(APP_TIME_ROOT_SLOT_SIZE - 1)

typedef void(*AppTimeoutCallback) (void*);

class CTimerWheel {
public:
    struct STimeNode {
        CQueueNode mLinker;
        AppTimeoutCallback mCallback;
        void* mCallbackData;
        s32 mTimeoutStep;           //Absolute timeout step
        s32 mCycleStep;             //cycle step,relatively
        s32 mMaxRepeat;
        STimeNode();
        ~STimeNode();
    };

    /**
    *@brief Constructor of time wheel.
    *@param time Current time stamp, in millisecond.
    *@param interval Internal working time interval(in millisecond),
    *@note Time wheel's time range is [0, (2^32 * interval)].
    */
    CTimerWheel(s64 time, s32 interval);

    ~CTimerWheel();

    void update(s64 currentTime);

    /**
    *@param period The timeout stamp, relative with current time.
    *@param repeat repeat times.-1=ever
    */
    void add(STimeNode& node, u32 period, s32 repeat);

    void add(STimeNode& node);

    bool remove(STimeNode& node);

    s32 getCurrentStep()const {
        return mCurrentStep;
    }

    void setCurrentStep(s32 step) {
        mCurrentStep = step;
    }

    /**
    * @param interval time interval of timewheel.
    */
    void setInterval(s32 interval) {
        mInterval = (interval > 0 ? interval : 1);
    }

    s32 getInterval()const {
        return mInterval;
    }

    s64 getCurrent()const {
        return mCurrent;
    }

    void setCurrent(s64 time) {
        mCurrent = time;
    }

    /**
    * @brief Clear all tasks, every task will be called back.
    */
    void clear();

    APP_INLINE static bool isTimeAfter32(s32 now, s32 timeout) {
        return (timeout - now < 0);
    }

    APP_INLINE static bool isTimeAfter64(s64 now, s64 timeout) {
        return (timeout - now < 0);
    }

private:
    void init();
    void initSlot(CQueueNode all[], u32 size);
    void clearSlot(CQueueNode all[], u32 size);

    enum {
        ESLOT0_MAX = (1 << (APP_TIME_ROOT_SLOT_BITS + APP_TIME_SLOT_BITS * 0)), //APP_TIME_ROOT_SLOT_SIZE,
        ESLOT1_MAX = (1 << (APP_TIME_ROOT_SLOT_BITS + APP_TIME_SLOT_BITS * 1)),
        ESLOT2_MAX = (1 << (APP_TIME_ROOT_SLOT_BITS + APP_TIME_SLOT_BITS * 2)),
        ESLOT3_MAX = (1 << (APP_TIME_ROOT_SLOT_BITS + APP_TIME_SLOT_BITS * 3)),
        //ESLOT_FORCE_32BIT = 0xFFFFFFFF
    };

    void innerUpdate();

    inline s32 getIndex(s32 jiffies, s32 level)const {
        return (jiffies >> (APP_TIME_ROOT_SLOT_BITS + level * APP_TIME_SLOT_BITS)) & APP_TIME_SLOT_MASK;
    }

    void innerAdd(STimeNode& node);

    void innerCascade(CQueueNode& head);

    s32 mInterval;
    s32 mCurrentStep;
    s64 mCurrent;
    CSpinlock mSpinlock;
    CQueueNode mSlot_0[APP_TIME_ROOT_SLOT_SIZE];
    CQueueNode mSlot_1[APP_TIME_SLOT_SIZE];
    CQueueNode mSlot_2[APP_TIME_SLOT_SIZE];
    CQueueNode mSlot_3[APP_TIME_SLOT_SIZE];
    CQueueNode mSlot_4[APP_TIME_SLOT_SIZE];
};

}//namespace irr

#endif //APP_CTIMERWHEEL_H