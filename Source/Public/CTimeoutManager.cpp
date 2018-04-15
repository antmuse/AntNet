#include "CTimeoutManager.h"
#include "IAppTimer.h"

namespace irr {


CTimeoutManager::CTimeoutManager(u32 step) :
    mStep(0 == step ? 1 : step),
    mRunning(false),
    mTimer(0, 1) {
}


CTimeoutManager::~CTimeoutManager() {
}


void CTimeoutManager::run() {
    for(; mRunning;) {
        mTimer.update(IAppTimer::getTime());
        CThread::sleep(mStep);
    }
}


void CTimeoutManager::start() {
    if(!mRunning) {
        mRunning = true;
        mTimer.setCurrent(IAppTimer::getTime());
        mThread.start(*this);
    }
}


void CTimeoutManager::stop() {
    if(mRunning) {
        mRunning = false;
        mThread.join();
    }
}



}//namespace irr
