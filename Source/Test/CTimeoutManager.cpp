#include "CTimeoutManager.h"
#include "CTimer.h"

namespace app {


CTimeoutManager::CTimeoutManager(u32 step) :
    mStep(0 == step ? 1 : (step > 5000 ? 5000 : step)),
    mRunning(false),
    mTimer(0LL, 100) {
}


CTimeoutManager::~CTimeoutManager() {
}


void CTimeoutManager::run() {
    s64 last = CTimer::getTime();
    s64 curr = last;
    for(; mRunning;) {
        mTimer.update(CTimer::getTime());
        curr = CTimer::getTime();
        last = curr - last;
        if(last < mStep) {
            CThread::sleep(mStep - last);
        }
        last = curr;
    }
}


void CTimeoutManager::start() {
    if(!mRunning) {
        mRunning = true;
        mTimer.setCurrent(CTimer::getTime());
        mThread.start(*this);
    }
}


void CTimeoutManager::stop() {
    if(mRunning) {
        mRunning = false;
        mThread.join();
        mTimer.clear();
    }
}



}//namespace app
