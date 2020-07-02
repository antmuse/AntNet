#include "CTimeManager.h"
#include "CTimer.h"

namespace app {
namespace core {


CTimeManager::CTimeManager(s32 step) :
    mStep(step < 1 ? 1 : (step > 2000 ? 2000 : step)),
    mRunning(false),
    mTimer(CTimer::getRelativeTime(), 100) {
}


CTimeManager::~CTimeManager() {
}


void CTimeManager::run() {
    s64 last = mTimer.getCurrent();
    s64 curr;
    for (; mRunning;) {
        curr = CTimer::getRelativeTime();
        mTimer.update(curr);
        last = curr - last;
        if (last < mStep) {
            CThread::sleep(mStep - last);
        }
        last = curr;
    }
}


void CTimeManager::start() {
    if (!mRunning) {
        mRunning = true;
        mTimer.setCurrent(CTimer::getRelativeTime());
        mThread.start(*this);
    }
}


void CTimeManager::stop() {
    if (mRunning) {
        mRunning = false;
        mThread.join();
        mTimer.clear();
    }
}



}//namespace core
}//namespace app
