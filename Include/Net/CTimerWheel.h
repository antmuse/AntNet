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
        u32 mState;
        u32 mTimeoutStep;//Absolute timeout step
        AppTimeoutCallback mCallback;
        void* mCallbackData;
        STimeNode();
        ~STimeNode();
    };
    struct STimeEvent {
        u32 mPeriod;
        s32 mRepeat;
        u32 mLastTime;
        AppTimeoutCallback mCallback;
        void* mCallbackData;
        CTimerWheel* mManger;
        STimeNode mNode;


        STimeEvent(AppTimeoutCallback cback, void* data);

        ~STimeEvent();

        // start timer: repeat <= 0 (infinite repeat)
        void start(CTimerWheel *mgr, u32 period, s32 repeat);

        // stop timer
        void stop();
    };

    /**
    *@brief Constructor of time wheel.
    *@param time Current time stamp, in millisecond.
    *@param interval Internal working time interval, time wheel's time range is (2^32 * interval).
    */
    CTimerWheel(u32 time, u32 interval);

    ~CTimerWheel();

    /**
    *@param time Current time, in millisecond.
    *@note It's ok for 64bit timestamp. just call update(u32(time64bit));
    */
    void update(u32 time);

    /**
    *@param period The timeout stamp, relative with current time, in millisecond.
    */
    void add(STimeNode& node, u32 period);

    void add(STimeNode& node);

    s32 remove(STimeNode& node);

    u32 getCurrentStep()const {
        return mCurrentStep;
    }

    u32 getInterval()const {
        return mInterval;
    }

    u32 getCurrent()const {
        return mCurrent;
    }

    void setCurrent(u32 time) {
        mCurrent = time;
    }

    void clear();

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

    inline u32 getIndex(u32 jiffies, u32 level)const;

    void innerAdd(STimeNode& node);

    void innerCascade(CQueueNode& head);

    u32 mInterval;
    u32 mCurrent;
    u32 mCurrentStep;
    CSpinlock mSpinlock;
    CQueueNode mSlot_0[APP_TIME_ROOT_SLOT_SIZE];
    CQueueNode mSlot_1[APP_TIME_SLOT_SIZE];
    CQueueNode mSlot_2[APP_TIME_SLOT_SIZE];
    CQueueNode mSlot_3[APP_TIME_SLOT_SIZE];
    CQueueNode mSlot_4[APP_TIME_SLOT_SIZE];
};

}//namespace irr

#endif //APP_CTIMERWHEEL_H