#include "CTimerWheel.h"

#ifndef APP_TIMER_MANAGER_LIMIT
#define APP_TIMER_MANAGER_LIMIT	300000		// 300 seconds
#endif


namespace irr {
//----------------------------------------------------------------
CTimerWheel::STimeNode::STimeNode() :
    mCycleStep(0),
    mMaxRepeat(1),
    mTimeoutStep(0),
    mCallback(0),
    mCallbackData(0) {
    //mLinker.init();
}


CTimerWheel::STimeNode::~STimeNode() {
    if(!mLinker.isEmpty()) {
        mLinker.delink();
    }
    mCallback = 0;
    mCallbackData = 0;
    mTimeoutStep = 0;
}


//---------------------------------------------------------------------
CTimerWheel::CTimerWheel(s64 millisec, s32 interval) :
    mCurrentStep(0),
    mCurrent(millisec),
    mInterval((interval > 0) ? interval : 1) {
    init();
}


CTimerWheel::~CTimerWheel() {
    clear();
}


void CTimerWheel::init() {
    initSlot(mSlot_0, APP_TIME_ROOT_SLOT_SIZE);
    initSlot(mSlot_1, APP_TIME_SLOT_SIZE);
    initSlot(mSlot_2, APP_TIME_SLOT_SIZE);
    initSlot(mSlot_3, APP_TIME_SLOT_SIZE);
    initSlot(mSlot_4, APP_TIME_SLOT_SIZE);
}


void CTimerWheel::clear() {
    mSpinlock.lock();
    clearSlot(mSlot_0, APP_TIME_ROOT_SLOT_SIZE);
    clearSlot(mSlot_1, APP_TIME_SLOT_SIZE);
    clearSlot(mSlot_2, APP_TIME_SLOT_SIZE);
    clearSlot(mSlot_3, APP_TIME_SLOT_SIZE);
    clearSlot(mSlot_4, APP_TIME_SLOT_SIZE);
    mSpinlock.unlock();
}


void CTimerWheel::initSlot(CQueueNode all[], u32 size) {
    for(u32 i = 0; i < size; i++) {
        all[i].init();
    }
}


void CTimerWheel::clearSlot(CQueueNode all[], u32 size) {
    STimeNode* node;
    AppTimeoutCallback fn;
    CQueueNode queued;
    for(u32 j = 0; j < size; j++) {
        if(!all[j].isEmpty()) {
            all[j].splitAndJoin(queued);

            mSpinlock.unlock(); //unlock

            while(!queued.isEmpty()) {
                node = APP_GET_VALUE_POINTER(queued.getNext(), STimeNode, mLinker);
                if(!node->mLinker.isEmpty()) {
                    node->mLinker.delink();
                }
                fn = node->mCallback;
                if(fn) {
                    fn(node->mCallbackData);
                }
            }//while

            mSpinlock.lock(); //lock
        }//if
    }//for
}


void CTimerWheel::add(STimeNode& node, u32 period, s32 repeat) {
    mSpinlock.lock();

    if(!node.mLinker.isEmpty()) {
        node.mLinker.delink();
    }
    u32 steps = (period + mInterval - 1) / mInterval;
    if(steps >= 0x70000000U) {//21 days max if mInterval=1 millisecond
        steps = 0x70000000U;
    }
    node.mCycleStep = static_cast<s32>(steps);
    node.mTimeoutStep = mCurrentStep + node.mCycleStep;
    node.mMaxRepeat = repeat;
    innerAdd(node);

    mSpinlock.unlock();
}


void CTimerWheel::add(STimeNode& node) {
    mSpinlock.lock();

    if(!node.mLinker.isEmpty()) {
        node.mLinker.delink();
    }
    innerAdd(node);

    mSpinlock.unlock();
}


bool CTimerWheel::remove(STimeNode& node) {
    mSpinlock.lock();

    if(!node.mLinker.isEmpty()) {
        node.mLinker.delink();
        mSpinlock.unlock(); //unlock
        return true;
    }

    mSpinlock.unlock(); //unlock
    return false;
}


void CTimerWheel::innerAdd(STimeNode& node) {
    s32 expires = node.mTimeoutStep;
    u32 idx = expires - mCurrentStep;
    if(idx < ESLOT0_MAX) {
        mSlot_0[expires & APP_TIME_ROOT_SLOT_MASK].pushBack(node.mLinker);
    } else if(idx < ESLOT1_MAX) {
        mSlot_1[getIndex(expires, 0)].pushBack(node.mLinker);
    } else if(idx < ESLOT2_MAX) {
        mSlot_2[getIndex(expires, 1)].pushBack(node.mLinker);
    } else if(idx < ESLOT3_MAX) {
        mSlot_3[getIndex(expires, 2)].pushBack(node.mLinker);
    } else if(static_cast<s32>(idx) < 0) {
        mSlot_0[mCurrentStep & APP_TIME_ROOT_SLOT_MASK].pushBack(node.mLinker);
    } else {
        mSlot_4[getIndex(expires, 3)].pushBack(node.mLinker);
    }
}


void CTimerWheel::innerCascade(CQueueNode& head) {
    CQueueNode queued;
    //queued.init();
    head.splitAndJoin(queued);
    STimeNode* node;
    while(!queued.isEmpty()) {
        node = APP_GET_VALUE_POINTER(queued.getNext(), STimeNode, mLinker);
        node->mLinker.delink();
        innerAdd(*node);
    }
}


void CTimerWheel::innerUpdate() {
    s32 index = mCurrentStep & APP_TIME_ROOT_SLOT_MASK;
    if(index == 0) {
        s32 i = getIndex(mCurrentStep, 0);
        innerCascade(mSlot_1[i]);
        if(i == 0) {
            i = getIndex(mCurrentStep, 1);
            innerCascade(mSlot_2[i]);
            if(i == 0) {
                i = getIndex(mCurrentStep, 2);
                innerCascade(mSlot_3[i]);
                if(i == 0) {
                    i = getIndex(mCurrentStep, 3);
                    innerCascade(mSlot_4[i]);
                }
            }
        }
    }//if

    mCurrentStep++;

    if(!mSlot_0[index].isEmpty()) {
        CQueueNode queued;
        //queued.init();
        mSlot_0[index].splitAndJoin(queued);
        STimeNode* node;
        AppTimeoutCallback fn;
        s32 repeat;
        mSpinlock.unlock(); //unlock

        while(!queued.isEmpty()) {
            node = APP_GET_VALUE_POINTER(queued.getNext(), STimeNode, mLinker);
            node->mLinker.delink();
            fn = node->mCallback;
            repeat = node->mMaxRepeat - (node->mMaxRepeat > 0 ? 1 : 0);
            if(fn) {
                fn(node->mCallbackData); //@note: node maybe deleted by this call.
            }
            if(0 != repeat && node->mCycleStep > 0) {
                node->mMaxRepeat = repeat;
                node->mTimeoutStep = mCurrentStep + node->mCycleStep;
                innerAdd(*node);
            }
        }

        mSpinlock.lock(); //lock
    }//if
}


// run timer events
void CTimerWheel::update(s64 millisec) {
    s64 limit = mInterval * 64LL;
    s64 diff = millisec - mCurrent;
    if(diff > APP_TIMER_MANAGER_LIMIT + limit) {
        mCurrent = millisec;
    } else if(diff < -APP_TIMER_MANAGER_LIMIT - limit) {
        mCurrent = millisec;
    }
    while(isTimeAfter64(millisec, mCurrent)) {
        mSpinlock.lock();
        innerUpdate();
        mCurrent += mInterval;
        mSpinlock.unlock();
    }
}


}//namespace irr