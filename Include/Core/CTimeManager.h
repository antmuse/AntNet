#ifndef APP_CTIMEOUTMANAGER_H
#define APP_CTIMEOUTMANAGER_H

#include "IRunnable.h"
#include "CThread.h"
#include "CTimerWheel.h"


namespace app {
namespace core {


class CTimeManager : public IRunnable {
public:
    /**
    * @brief Create a manager of timeout events.
    * @param timeinterval The time manager update time interval, in millisecond.
    */
    CTimeManager(s32 timeinterval);

    virtual ~CTimeManager();

    virtual void run();

    void start();

    /**
    * @brief Stop manager without clear any tasks.
    */
    void stop();

    /**
    * @brief Change the update interval.
    * @param timeinterval in millisecond.
    */
    void setInterval(s32 timeinterval) {
        mStep = timeinterval < 1 ? 1 : (timeinterval > 2000 ? 2000 : timeinterval);
    }

    CTimerWheel& getTimeWheel() {
        return mTimer;
    }

private:
    bool mRunning;
    s32 mStep;
    CTimerWheel mTimer;
    CThread mThread;
};


}//namespace core
}//namespace app

#endif //APP_CTIMEOUTMANAGER_H