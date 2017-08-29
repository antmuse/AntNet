/**
*@file CThreadPool.h
*@brief This file defined a pool of threads.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_CTHREADPOOL_H
#define APP_CTHREADPOOL_H

#include "irrList.h"
#include "irrArray.h"
#include "IRunnable.h"
#include "CThread.h"

namespace irr {

    /**
    *@class CThreadPool
    *@brief A thread pool.
    */
    class CThreadPool {
    private:
        class CThreadWorker : public IRunnable {
        public:
            CThreadWorker(CThreadPool* iPool,u32 id) : mRunning(false),
                mPool(iPool),
                mID(id),
                mCallTask(0,0),
                mTask(0) {
            }

            virtual ~CThreadWorker() {
                stop();
            }

            virtual void run();


            void start() {
                if (!mRunning) {
                    mRunning = true;
                    mThread.start(*this);
                }
            }


            void stop() {
                if (mRunning) {
                    mRunning = false;
                    mThread.join();
                }
            }

            void setTask(IRunnable* it) {
                mTask = it;
            }

            void setTask(AppCallable iFunc, void* iData) {
                mCallTask.mData = iData;				//step 1
                mCallTask.mCallback = iFunc;		//step 2
            }

            u32 getID()const {
                return mID;
            }

        private:
            CThreadWorker(){
            }

            CThreadWorker(const CThreadWorker& other){
            }

            CThreadWorker& operator = (const CThreadWorker& other){
            }

            bool mRunning;
            u32 mID;
            IRunnable* mTask;
            volatile SCallbackData mCallTask;
            CThreadPool* mPool;
            CThread mThread;
        };


    public:
        CThreadPool(u32 iThreadCount);

        ~CThreadPool();

        /**
        *@param it 0: no limit
        */
        void setMaxHoldTasks(u32 it){
            mMaxTasks = it;
        }

        void start();

        void stop();

        bool start(AppCallable iFunc, void* iData = 0);

        bool start(IRunnable* it);

        void onTask(CThreadWorker* it);


    private:
        CThreadPool() {
        }

        void creatThread(u32 iCount);

        void removeAll();


        bool mRunning;
        u32 mThreadCount;
        u32 mMaxTasks;          ///<0: disable limit, else limit the max tasks saved in list. default: 0
        CMutex mMutex;
        core::list<IRunnable*> mHoldTasks;
        core::list<SCallbackData> mHoldCallTasks;
        core::array<CThreadWorker*> mBusyWorker;
        core::array<CThreadWorker*> mIdleWorker;
    };

}//irr


#endif	/* APP_CTHREADPOOL_H */