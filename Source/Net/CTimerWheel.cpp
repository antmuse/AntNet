#include "CTimerWheel.h"
#include "CMutex.h"

#ifndef APP_TIMER_MANAGER_LIMIT
#define APP_TIMER_MANAGER_LIMIT	300000		// 300 seconds
#endif

#define APP_TIMER_NODE_VALID		0x2277
#define APP_TIMER_NODE_INVALID		0x7722


namespace irr {
//----------------------------------------------------------------
CTimerWheel::STimeNode::STimeNode() :
    mTimeoutStep(0),
    mCallback(0),
    mCallbackData(0),
    mState(APP_TIMER_NODE_VALID) {
    //mLinker.init();
}


CTimerWheel::STimeNode::~STimeNode() {
    if(mState != APP_TIMER_NODE_VALID) {
        APP_ASSERT(0);
        return;
    }
    if(!mLinker.isEmpty()) {
        mLinker.delink();
    }
    mState = APP_TIMER_NODE_INVALID;
    mCallback = 0;
    mCallbackData = 0;
    mTimeoutStep = 0;
}


//---------------------------------------------------------------------
CTimerWheel::CTimerWheel(u32 millisec, u32 interval) :
    mMutex(0),
    mCurrentStep(0),
    mCurrent(millisec),
    mInterval((interval > 0) ? interval : 1) {
    mMutex = new CMutex();
}


CTimerWheel::~CTimerWheel() {
    delete mMutex;
}


void CTimerWheel::add(STimeNode& node, u32 period) {
    if(node.mState != APP_TIMER_NODE_VALID) {
        APP_ASSERT(0);
        return;
    }
    if(!node.mLinker.isEmpty()) {
        node.mLinker.delink();
    }
    u32 steps = (period + mInterval - 1) / mInterval;
    if(steps >= 0x70000000) {
        steps = 0x70000000;
    }
    node.mTimeoutStep = mCurrentStep + steps;
    innerAdd(node);
}


void CTimerWheel::add(STimeNode& node) {
    if(node.mState != APP_TIMER_NODE_VALID) {
        APP_ASSERT(0);
        return;
    }
    if(!node.mLinker.isEmpty()) {
        node.mLinker.delink();
    }
    innerAdd(node);
}


s32 CTimerWheel::remove(STimeNode& node) {
    if(node.mState != APP_TIMER_NODE_VALID) {
        APP_ASSERT(0);
        return -1;
    }
    if(!node.mLinker.isEmpty()) {
        node.mLinker.delink();
        return 1;
    }
    return 0;
}


void CTimerWheel::innerAdd(STimeNode& node) {
    u32 expires = node.mTimeoutStep;
    u32 idx = expires - mCurrentStep;
    if(idx < ESLOT0_MAX) {
        mSlot_0.pushBack(expires & APP_TIME_ROOT_SLOT_MASK, node.mLinker);
    } else if(idx < ESLOT1_MAX) {
        mSlot_1.pushBack(getIndex(expires, 0), node.mLinker);
    } else if(idx < ESLOT2_MAX) {
        mSlot_2.pushBack(getIndex(expires, 1), node.mLinker);
    } else if(idx < ESLOT3_MAX) {
        mSlot_3.pushBack(getIndex(expires, 2), node.mLinker);
    } else if((s32) idx < 0) {
        mSlot_0.pushBack(mCurrentStep & APP_TIME_ROOT_SLOT_MASK, node.mLinker);
    } else {
        mSlot_4.pushBack(getIndex(expires, 3), node.mLinker);
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


inline u32 CTimerWheel::getIndex(u32 jiffies, u32 level)const {
    return (jiffies >> (APP_TIME_ROOT_SLOT_BITS + level * APP_TIME_SLOT_BITS)) & APP_TIME_SLOT_MASK;
}


void CTimerWheel::innerUpdate() {
    u32 index = mCurrentStep & APP_TIME_ROOT_SLOT_MASK;
    if(index == 0) {
        u32 i = getIndex(mCurrentStep, 0);
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
        void* data;
        while(!queued.isEmpty()) {
            node = APP_GET_VALUE_POINTER(queued.getNext(), STimeNode, mLinker);
            fn = node->mCallback;
            data = node->mCallbackData;
            node->mLinker.delink();
            if(fn) fn(data);
        }
    }//if
}


// run timer events
void CTimerWheel::update(u32 millisec) {
    s32 limit = (s32) mInterval * 64;
    s32 diff = (s32) (millisec - mCurrent);
    if(diff > APP_TIMER_MANAGER_LIMIT + limit) {
        mCurrent = millisec;
    } else if(diff < -APP_TIMER_MANAGER_LIMIT - limit) {
        mCurrent = millisec;
    }
    while((s32) (millisec - mCurrent) >= 0) {
        innerUpdate();
        mCurrent += mInterval;
    }
}


//---------------------------------------------------------------------
void AppTimeEventCallback(void* data) {
    CTimerWheel::STimeEvent* evt = (CTimerWheel::STimeEvent*) data;
    CTimerWheel* mgr = evt->mManger;
    u32 current = mgr->getCurrent();
    s32 count = 0;
    bool stop = false;
    while(current >= evt->mLastTime) {
        count++;
        evt->mLastTime += evt->mPeriod;
        if(evt->mRepeat == 1) {
            stop = true;
            break;
        }
        if(evt->mRepeat > 1) {
            evt->mRepeat--;
        }
    }

    if(stop) {
        evt->stop();
    } else {
        mgr->add(evt->mNode, evt->mPeriod);
    }

    if(evt->mCallback) {
        for(; count > 0; count--) {
            evt->mCallback(evt->mCallbackData);
        }
    }
}


CTimerWheel::STimeEvent::STimeEvent(AppTimeoutCallback cback, void* data) :
    mRepeat(0),
    mLastTime(0),
    mPeriod(0),
    mManger(0),
    mCallback(cback),
    mCallbackData(data) {
    mNode.mCallback = AppTimeEventCallback;
    mNode.mCallbackData = this;
}


CTimerWheel::STimeEvent::~STimeEvent() {
    mCallback = 0;
    mCallbackData = 0;
    mManger = 0;
    mPeriod = 0;
    mLastTime = 0;
    mRepeat = 0;
}


void CTimerWheel::STimeEvent::start(CTimerWheel* mgr, u32 period, s32 repeat) {
    if(mgr) {
        stop();
        mManger = mgr;
        mPeriod = period;
        mRepeat = repeat;
        mLastTime = mgr->getCurrent() + period;
        mgr->add(mNode, period);
    }
}


void CTimerWheel::STimeEvent::stop() {
    if(mManger) {
        mManger->remove(mNode);
        mManger = 0;
    }
}


}//namespace irr
