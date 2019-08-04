#include "CNetManager.h"
#include "IAppLogger.h"
#include "IAppTimer.h"
#include "CThread.h"
#include "CNetClientTCP.h"
#include "CNetClientUDP.h"
#include "CNetClientHttp.h"
#include "CNetServerTCP.h"
#include "CNetService.h"
#include "CNetServerSeniorUDP.h"

namespace irr {
namespace net {

CNetManager::CNetManager() :
    mRunning(false),
    mThread(0) {
    mThread = new CThread();
}


CNetManager::~CNetManager() {
    stop();
    delete mThread;
}


CNetManager& CNetManager::getInstance() {
    static CNetManager it;
    return it;
}


void CNetManager::run() {
    APP_LOG(ELOG_DEBUG, "CNetManager::run", "%s", "enter");

    u64 time;
    for(; mRunning;) {
        time = IAppTimer::getTime();

        //update TCP
        for(u32 i = 0; i < mAllConnectionTCP.size(); ) {
            if(mAllConnectionTCP[i]->update(time)) {
                ++i;
            } else {
                //mAllConnectionTCP[i]->stop();
                delete mAllConnectionTCP[i];
                mAllConnectionTCP[i] = mAllConnectionTCP.getLast();
                mAllConnectionTCP.set_used(mAllConnectionTCP.size() - 1);
                IAppLogger::log(ELOG_INFO, "CNetManager::run", "TCP node quit, leftover = [%u]", mAllConnectionUDP.size());
            }
        }

        //update UDP
        for(u32 i = 0; i < mAllConnectionUDP.size(); ++i) {
            if(mAllConnectionUDP[i]->update(time)) {
                ++i;
            } else {
                //mAllConnectionUDP[i]->stop();
                delete mAllConnectionUDP[i];
                mAllConnectionUDP[i] = mAllConnectionUDP.getLast();
                mAllConnectionUDP.set_used(mAllConnectionUDP.size() - 1);
                IAppLogger::log(ELOG_INFO, "CNetManager::run", "UDP node quit, leftover = [%u]", mAllConnectionUDP.size());
            }
        }

        CThread::sleep(10);
    }//for

    stopAll();

    APP_LOG(ELOG_DEBUG, "CNetManager::run", "%s", "quit");
}


bool CNetManager::start() {
    if(mRunning) return false;

    mRunning = true;
    mThread->start(*this);
    return true;
}


void CNetManager::stopAll() {
    for(u32 i = 0; i < mAllConnectionTCP.size(); ++i) {
        mAllConnectionTCP[i]->stop();
        delete mAllConnectionTCP[i];
    }
    for(u32 i = 0; i < mAllConnectionUDP.size(); ++i) {
        mAllConnectionUDP[i]->stop();
        delete mAllConnectionUDP[i];
    }
    APP_LOG(ELOG_DEBUG, "CNetManager::stopAll", "TCP %u", mAllConnectionTCP.size());
    APP_LOG(ELOG_DEBUG, "CNetManager::stopAll", "UDP %u", mAllConnectionUDP.size());
    mAllConnectionTCP.set_used(0);
    mAllConnectionUDP.set_used(0);
}


bool CNetManager::stop() {
    if(!mRunning) return false;

    mRunning = false;
    mThread->join();
    return true;
}


INetClient* CNetManager::getClientTCP(u32 index) {
    if(index >= mAllConnectionTCP.size()) {
        return nullptr;
    }
    return mAllConnectionTCP[index];
}


INetClient* CNetManager::getClientUDP(u32 index) {
    if(index >= mAllConnectionUDP.size()) {
        return nullptr;
    }
    return mAllConnectionUDP[index];
}


INetClient* CNetManager::addClient(ENetNodeType type, INetEventer* iEvt) {
    CAutoLock alock(mMutex);
    INetClient* it = nullptr;

    switch(type) {
    case ENET_TCP_CLIENT:
        it = new CNetClientTCP();
        mAllConnectionTCP.push_back(it);
        break;

    case ENET_UDP_CLIENT:
        it = new CNetClientUDP();
        mAllConnectionUDP.push_back(it);
        break;
    }
    if(it) {
        it->setNetEventer(iEvt);
    }
    return it;
}


INetServer* CNetManager::createServer(ENetNodeType type, INetEventer* iEvt) {
    CAutoLock alock(mMutex);
    INetServer* it = nullptr;

    switch(type) {
    case ENET_TCP_SERVER:
        it = 0;// new CNetServiceTCP();
        break;

    case ENET_UDP_SERVER:
        it = new CNetServerSeniorUDP();
        break;

    case ENET_TCP_SERVER_LOW:
        it = new CNetServerTCP();
        break;
    }
    if(it) {
        it->setNetEventer(iEvt);
    }
    return it;
}


INetClient* CNetManager::createClient(ENetNodeType type, INetEventer* iEvt) {
    INetClient* it = nullptr;

    switch(type) {
    case ENET_TCP_CLIENT:
        it = new CNetClientTCP();
        break;

    case ENET_UDP_CLIENT:
        it = new CNetClientUDP();
        break;
    }
    if(it) {
        it->setNetEventer(iEvt);
    }
    return it;
}


}// end namespace net
}// end namespace irr
