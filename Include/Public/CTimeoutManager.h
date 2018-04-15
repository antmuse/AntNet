#ifndef APP_CTIMEOUTMANAGER_H
#define APP_CTIMEOUTMANAGER_H

#include "IRunnable.h"
#include "CThread.h"
#include "CTimerWheel.h"


namespace irr {


class CTimeoutManager : public IRunnable {
public:
    /**
    * @brief Create a manager of timeout events.
    * @param timeinterval The time manager update time interval, in millisecond.
    */
    CTimeoutManager(u32 timeinterval);

    virtual ~CTimeoutManager();

    virtual void run();

    void start();

    void stop();

    /**
    * @brief Change the update interval.
    * @param timeinterval Time manager update time interval, in millisecond.
    */
    void setInterval(u32 timeinterval) {
        mStep = timeinterval;
    }

    CTimerWheel& getTimeWheel() {
        return mTimer;
    }

private:
    bool mRunning;
    u32 mStep;
    CTimerWheel mTimer;
    CThread mThread;
};


}//namespace irr

#endif //APP_CTIMEOUTMANAGER_H