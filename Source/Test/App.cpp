#include <stdio.h>
#include "HAtomicOperator.h"
#include "SNetAddress.h"
#include "INetManager.h"
#include "IAppLogger.h"
#include "CNetServerAcceptor.h"
#include "CNetClientSeniorTCP.h"
#include "CDefaultNetEventer.h"
#include "CNetPing.h"
#include "CTimeoutManager.h"
#include "IAppTimer.h"
#include "CAtomicValue32.h"


//#include "CNetSynPing.h"


#ifdef   APP_PLATFORM_WINDOWS
#ifdef   APP_DEBUG
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:mainWCRTStartup")
#else       //for release version
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainWCRTStartup")
#endif  //release version

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif  //APP_PLATFORM_WINDOWS


namespace irr {

void AppQuit() {
    /*printf("@Please any key to quit\n");
    * system("PAUSE");*/
    irr::c8 key = '\0';
    while('*' != key) {
        printf("@Please input [*] to quit\n");
        scanf("%c", &key);
    }
    printf("@App quitting...\n");
}


void AppStartServer() {
    //net::CDefaultNetEventer evt;
    net::CNetServerAcceptor accpetor;
    net::SNetAddress addr(9981);
    accpetor.setLocalAddress(addr);
    accpetor.start();
    AppQuit();
    accpetor.stop();
}


void AppStartClient() {
    const s32 max = 1;
    net::CDefaultNetEventer evt[max];
    net::INetSession* session[max];
    net::CNetClientSeniorTCP chub;
    chub.start();
    net::SNetAddress addr("127.0.0.1", 9981);
    s32 i;
    for(i = 0; i < max; ++i) {
        evt[i].setHub(&chub);
        session[i] = chub.getSession(&evt[i]);
        if(session[i]) {
            evt[i].setSession(session[i]);
            session[i]->connect(addr);
        } else {
            break;
        }
        //CThread::sleep(100);
    }

    AppQuit();
    chub.stop();
}


void AppStartPing() {
    irr::net::CNetPing rpin;
    bool got = rpin.ping("61.135.169.121", 2, 1000);
    printf("ping 61.135.169.121 = %s\n", got ? "yes" : "no");
    printf("-------------------------------------\n");
}



void AppStartTimerWheel() {
    class CTimeAdder : public IRunnable {
    public:
        CTimeAdder(CTimerWheel& it, u32 maxStep) :
            mIndex(0),
            mMaxStep(maxStep < 5 ? 5 : maxStep),
            mRunning(false),
            mTimer(it) {
        }
        static void timeout(void* nd) {
            SNode* node = (SNode*)nd;
            s32 leftover = AppAtomicDecrementFetch(node->mAdder->getCount());
            printf("timeout: id = %d, time = %u, leftover = %d\n", 
                node->mID, node->mTimeNode.mTimeoutStep, leftover);
            delete node;
        }
        virtual void run() {
            for(; mRunning;) {
                add();
                CThread::sleep(mMaxStep);
            }
        }
        s32* getCount() {
            return &mIndex;
        }
        void start() {
            if(!mRunning) {
                mRunning = true;
                mThread.start(*this);
            }
        }
        void stop() {
            if(mRunning) {
                mRunning = false;
                mThread.join();
            }
        }
    private:
        void add() {
            static s32 id = 0;
            SNode* node = new SNode();
            node->mID = id++;
            node->mAdder = this;
            node->mTimeNode.mCallback = CTimeAdder::timeout;
            node->mTimeNode.mCallbackData = node;
            AppAtomicIncrementFetch(&mIndex);
            mTimer.add(node->mTimeNode, 1 * 1000);
        }
        struct SNode {
            s32 mID;
            CTimeAdder* mAdder;
            CTimerWheel::STimeNode mTimeNode;
        };
        bool mRunning;
        u32 mMaxStep;
        s32 mIndex;
        CTimerWheel& mTimer;
        CThread mThread;
    };

    CTimeoutManager tmanager(22);
    CTimeAdder tadder(tmanager.getTimeWheel(), 50);
    //tmanager.getTimeWheel().setInterval(0x10000000);
    tmanager.start();
    tadder.start();
    //CThread::sleep(10 * 1000);
    AppQuit();
    printf("--------------clear time wheel--------------\n");
    tmanager.getTimeWheel().clear();
    tadder.stop();
    tmanager.stop();

    printf("finished, leftover = %d\n", *tadder.getCount());
    printf("--------------------------------------------\n");
}

}//namespace irr


int main(int argc, char** argv) {
    //irr::IAppLogger::getInstance();
    printf("@1 = Net Server\n");
    printf("@2 = Net Client\n");
    printf("@3 = Net Ping\n");
    printf("@4 = Time wheel\n");
    printf("@Input menu id = ");
    irr::u32 key;
    scanf("%u", &key);
    switch(key) {
    case 1:
        irr::AppStartServer();
        break;
    case 2:
        irr::AppStartClient();
        break;
    case 3:
        irr::AppStartPing();
        break;
    case 4:
        irr::AppStartTimerWheel();
        break;
    }
    irr::AppQuit();
    return 0;
}

