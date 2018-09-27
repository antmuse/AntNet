#include <stdio.h>
#include "HAtomicOperator.h"
#include "CNetAddress.h"
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
    net::CNetConfig* config = new net::CNetConfig();
    config->mReuse = true;
    config->mMaxPostAccept = 8;
    config->mMaxFatchEvents = 28;
    config->mMaxContext = 20000;
    config->mPollTimeout = 5000;
    config->print();

    net::CDefaultNetEventer evt;
    net::CNetServerAcceptor accpetor(config);
    config->drop();
    evt.setServer(&accpetor);
    net::CNetAddress addr(9981);
    accpetor.setLocalAddress(addr);
    accpetor.setEventer(&evt);
    accpetor.start();
    AppQuit();
    accpetor.stop();
}

void AppStartClient() {
    net::CNetConfig* config = new net::CNetConfig();
    config->mMaxFatchEvents = 128;
    config->mMaxContext = 16;
    config->mPollTimeout = 5000;
    config->print();

    const s32 max = 10;
    net::CDefaultNetEventer evt[max];
    u32 session[max];
    net::CNetClientSeniorTCP chub(config);
    config->drop();

    chub.start();
    //net::CNetAddress addr("221.204.177.67", 9981);
    net::CNetAddress addr("127.0.0.1", 9981);
    s32 i;
    for(i = 0; i < max; ++i) {
        evt[i].setHub(&chub);
        evt[i].setAutoConnect(true);
        session[i] = chub.getSession(&evt[i]);
        if(session[i]) {
            evt[i].setSession(session[i]);
            chub.connect(session[i], addr);
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
        CTimeAdder(CTimerWheel& it, u32 maxStep, s32 repeat) :
            mIndex(0),
            mMaxRepeat(repeat),
            mMaxStep(maxStep < 5 ? 5 : maxStep),
            mRunning(false),
            mTimer(it) {
        }
        static void timeout(void* nd) {
            SNode* node = (SNode*) nd;
            s32 leftover = AppAtomicDecrementFetch(node->mAdder->getCount());
            if(node->mDeleteFlag) {
                printf("CTimeAdder::timeout>> delete[Adder=%p],[id = %d],[time = %llu],[leftover = %d]\n",
                    node->mAdder, node->mID, node->mTimeNode.mTimeoutStep, leftover);
                delete node;
            } else {
                printf("CTimeAdder::timeout>> repeat[Adder=%p],[id = %d],[time = %llu],[leftover = %d]\n",
                    node->mAdder, node->mID, node->mTimeNode.mTimeoutStep, leftover);
            }
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
                if(mMaxRepeat > 0) {
                    mThread.start(*this);
                } else {
                    mRepeatNode.mDeleteFlag = false;
                    mRepeatNode.mID = -1;
                    mRepeatNode.mAdder = this;
                    mRepeatNode.mTimeNode.mCallback = CTimeAdder::timeout;
                    mRepeatNode.mTimeNode.mCallbackData = &mRepeatNode;
                    mTimer.add(mRepeatNode.mTimeNode, 1 * 1000, mMaxRepeat);
                }
            }
        }
        void stop() {
            if(mRunning) {
                mRunning = false;
                if(mMaxRepeat > 0) {
                    mThread.join();
                } else {
                    mTimer.remove(mRepeatNode.mTimeNode);
                }
            }
        }
    private:
        void add() {
            static s32 id = 0;
            SNode* node = new SNode();
            node->mDeleteFlag = true;
            node->mID = id++;
            node->mAdder = this;
            node->mTimeNode.mCallback = CTimeAdder::timeout;
            node->mTimeNode.mCallbackData = node;
            AppAtomicIncrementFetch(&mIndex);
            mTimer.add(node->mTimeNode, 1 * 1000, mMaxRepeat);
        }
        struct SNode {
            bool mDeleteFlag;
            s32 mID;
            CTimeAdder* mAdder;
            CTimerWheel::STimeNode mTimeNode;
        };
        bool mRunning;
        u32 mMaxStep;
        s32 mIndex;
        s32 mMaxRepeat;
        SNode mRepeatNode;
        CTimerWheel& mTimer;
        CThread mThread;
    };//CTimeAdder

    CTimeoutManager tmanager(5);
    CTimeAdder tadder1(tmanager.getTimeWheel(), 50, 1);
    tmanager.start();
    tadder1.start();

    AppQuit();
    tadder1.stop();
    tmanager.stop();
    printf("--------------clear time wheel--------------\n");
    printf("finished tadder1 = %p, leftover = %d\n", &tadder1, *tadder1.getCount());
    printf("--------------------------------------------\n");
}

}//namespace irr


int main(int argc, char** argv) {
    irr::IAppLogger::getInstance().addReceiver(1);
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

