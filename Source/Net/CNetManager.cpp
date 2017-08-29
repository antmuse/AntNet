#include "CNetManager.h"
#include "IAppLogger.h"
#include "IAppTimer.h"
#include "CThread.h"
#include "CNetClientTCP.h"
#include "CNetClientUDP.h"
#include "CNetServerSeniorTCP.h"
#include "CNetServerSeniorUDP.h"
#include "CNetServerNormalTCP.h"

namespace irr {
    namespace net {
        
        INetManager* AppGetNetManagerInstance(){
            return CNetManager::getInstance();
        }


        CNetManager::CNetManager() : mRunning(false),
            mThread(0){
                mThread = new CThread();
        }


        CNetManager::~CNetManager(){
            stop();
            delete mThread;
        }


        INetManager* CNetManager::getInstance(){
            static CNetManager it;
            return &it;
        }


        void CNetManager::run(){
            APP_LOG(ELOG_DEBUG, "CNetManager::run", "enter");

            u32 time;
            for(;mRunning;){
                time = IAppTimer::getTime();

                //update TCP
                for(u32 i=0; i<mAllConnectionTCP.size(); ){
                    if(mAllConnectionTCP[i]->update(time)){
                        ++i;
                    }else{
                        //mAllConnectionTCP[i]->stop();
                        delete mAllConnectionTCP[i];
                        mAllConnectionTCP[i] = mAllConnectionTCP.getLast();
                        mAllConnectionTCP.set_used(mAllConnectionTCP.size()-1);
                        IAppLogger::log(ELOG_INFO, "CNetManager::run", "TCP node quit, leftover = [%u]", mAllConnectionUDP.size());
                    }
                }

                //update UDP
                for(u32 i=0; i<mAllConnectionUDP.size(); ++i){
                    if(mAllConnectionUDP[i]->update(time)){
                        ++i;
                    }else{
                        //mAllConnectionUDP[i]->stop();
                        delete mAllConnectionUDP[i];
                        mAllConnectionUDP[i] = mAllConnectionUDP.getLast();
                        mAllConnectionUDP.set_used(mAllConnectionUDP.size()-1);
                        IAppLogger::log(ELOG_INFO, "CNetManager::run", "UDP node quit, leftover = [%u]", mAllConnectionUDP.size());
                    }
                }

                CThread::sleep(10);
            }//for

            stopAll();

            APP_LOG(ELOG_DEBUG, "CNetManager::run", "quit");
        }


        bool CNetManager::start(){
            if(mRunning) return false;

            mRunning = true;
            mThread->start(*this);
            return true;
        }


        void CNetManager::stopAll(){
            for(u32 i=0; i<mAllConnectionTCP.size(); ++i){
                mAllConnectionTCP[i]->stop();
                delete mAllConnectionTCP[i];
            }
            for(u32 i=0; i<mAllConnectionUDP.size(); ++i){
                mAllConnectionUDP[i]->stop();
                delete mAllConnectionUDP[i];
            }
            APP_LOG(ELOG_DEBUG, "CNetManager::stopAll", "TCP %u", mAllConnectionTCP.size());
            APP_LOG(ELOG_DEBUG, "CNetManager::stopAll", "UDP %u", mAllConnectionUDP.size());
            mAllConnectionTCP.set_used(0);
            mAllConnectionUDP.set_used(0);
        }


        bool CNetManager::stop(){
            if(!mRunning) return false;

            mRunning = false;
            mThread->join();
            return true;
        }


        INetClient* CNetManager::getClientTCP(u32 index){
            if(index>=mAllConnectionTCP.size()) return 0;

            return mAllConnectionTCP[index];
        }


        INetClient* CNetManager::getClientUDP(u32 index){
            if(index>=mAllConnectionUDP.size()) return 0;

            return mAllConnectionUDP[index];
        }


        INetClient* CNetManager::addClientTCP(){
            CAutoLock alock(mMutex);
            INetClient* it = new CNetClientTCP();
            mAllConnectionTCP.push_back(it);
            return it;
        }


        INetClient* CNetManager::addClientUDP(){
            CAutoLock alock(mMutex);
            INetClient* it = new CNetClientUDP();
            mAllConnectionUDP.push_back(it);
            return it;
        }


        INetServer* CNetManager::createServerSeniorTCP(){
            CAutoLock alock(mMutex);
            INetServer* it = new CNetServerSeniorTCP();
            return it;
        }


        INetServer* CNetManager::createServerSeniorUDP(){
            CAutoLock alock(mMutex);
            INetServer* it = new CNetServerSeniorUDP();
            return it;
        }


        INetServer* CNetManager::createServerNormalTCP(){
            CAutoLock alock(mMutex);
            INetServer* it = new CNetServerNormalTCP();
            return it;
        }


        INetServer* CNetManager::createServerNormalUDP(){
            CAutoLock alock(mMutex);
            INetServer* it = 0; //TODO>> new CNetServerUDP();
            return it;
        }


    }// end namespace net
}// end namespace irr
