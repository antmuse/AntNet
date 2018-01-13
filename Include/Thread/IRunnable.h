/**
*@file IRunnable.h
*@brief This file defined the runnable interface of threads.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_IRUNNABLE_H
#define APP_IRUNNABLE_H

namespace irr {

/**
*@class IRunnable
*@brief A runnable interface of threads.
*/

class IRunnable {
public:
    IRunnable() {
    }

    virtual ~IRunnable() {
    }

    /**
    *@brief This function run in thread.
    */
    virtual void run() = 0;
};


} //namespace irr 

#endif	/* APP_IRUNNABLE_H */
