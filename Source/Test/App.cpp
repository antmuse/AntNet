#include <stdio.h>
#include "CNetAddress.h"
#include "IAppLogger.h"
#include "CNetServerAcceptor.h"
#include "CNetService.h"
#include "CNetPing.h"
#include "CNetSynPing.h"
#include "CNetEchoServer.h"
#include "CNetEchoClient.h"
#include "CTimeoutManager.h"


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
    //system("PAUSE");
    irr::c8 key = '\0';
    while('*' != key) {
        printf("@Please input [*] to quit");
        scanf("%c", &key);
    }
}

void AppRunEchoServer() {
    net::CNetConfig* config = new net::CNetConfig();
    config->mReuse = true;
    config->mMaxPostAccept = 8;
    config->mMaxFetchEvents = 28;
    config->mMaxContext = 200;
    config->mPollTimeout = 5;
    config->mSessionTimeout = 30000;
    config->mMaxWorkThread = 5;
    config->check();
    config->print();

    net::CNetAddress addr(9981);
    net::CNetEchoServer evt;
    net::CNetServerAcceptor accpetor;
    evt.setServer(&accpetor);//accpetor.setEventer(&evt);
    accpetor.setLocalAddress(addr);
    accpetor.start(config);
    config->drop();
    //AppQuit();
    irr::c8 key = '\0';
    while('*' != key) {
        printf("@Please input [*] to quit");
        scanf("%c", &key);
        IAppLogger::log(ELOG_INFO, "AppRunEchoServer",
            "link=%u/%u, send = %uKb, sent=%uKb+%uKb, recv=%uKb",
            evt.getDislinkCount(),
            evt.getLinkCount(),
            evt.getSendSize() >> 10,
            evt.getSentSuccessSize() >> 10,
            evt.getSentFailSize() >> 10,
            evt.getRecvSize() >> 10);
    }
    accpetor.stop();
}

void AppRunEchoClient() {
    net::CNetConfig* config = new net::CNetConfig();
    config->mMaxWorkThread = 5;
    config->mMaxFetchEvents = 128;
    config->mMaxContext = 16;
    config->mPollTimeout = 5;
    config->mSessionTimeout = 20000;
    config->check();
    config->print();

    c8 sip[32] = {0};
    printf("@Please input server's IP = ");
    scanf("%s", sip);
    if(strlen(sip) < 4) {
        memcpy(sip, "127.0.0.1", sizeof("127.0.0.1"));
    }
    printf("@Will connect to server = %s:9981\n", sip);
    printf("@How many Mb do you want to send = ");
    s32 mMbyte = 1024 * 1024 * 1024;
    scanf("%d", &mMbyte);

    printf("@How many tcp do you want = ");
    s32 max = 10;
    scanf("%d", &max);

    net::CNetEchoClient::initData("I am client for echo server test.", mMbyte);
    net::CNetEchoClient* evt = new net::CNetEchoClient[max];
    net::CNetServiceTCP chub;
    chub.start(config);
    config->drop();

    net::CNetAddress addr(sip, 9981);
    s32 i;
    for(i = 0; i < max; ++i) {
        evt[i].setHub(&chub);
        evt[i].setAutoConnect(true);
        if(0 == chub.connect(addr, &evt[i])) {
            break;
        }
        //CThread::sleep(100);
    }

    //AppQuit();
    irr::c8 key = '\0';
    while('*' != key) {
        printf("@Please input [*] to quit");
        scanf("%c", &key);
        IAppLogger::log(ELOG_INFO, "AppRunEchoClient",
            "request=%u+%u/%u,send=%u+%u(%uKb+%uKb), receive=%u+%u=%uKb, tick=%u-%u=%u",
            net::CNetEchoClient::mSendRequest,
            net::CNetEchoClient::mSendRequestFail,
            net::CNetEchoClient::mMaxSendPackets,
            net::CNetEchoClient::mSendSuccess,
            net::CNetEchoClient::mSendFail,
            net::CNetEchoClient::mSendSuccessBytes >> 10,
            net::CNetEchoClient::mSendFailBytes >> 10,
            net::CNetEchoClient::mRecvCount,
            net::CNetEchoClient::mRecvBadCount,
            net::CNetEchoClient::mRecvBytes >> 10,
            net::CNetEchoClient::mTickRequest,
            net::CNetEchoClient::mTickRequestFail,
            net::CNetEchoClient::mTickRecv
            );
    }

    for(i = 0; i < max; ++i) {
        evt[i].setAutoConnect(false);
    }
    chub.stop();
    delete[] evt;
}

void AppStartPing() {
    irr::net::CNetPing rpin;
    bool got = rpin.ping("61.135.169.121", 5, 1000);
    printf("ping 61.135.169.121 = %s\n", got ? "yes" : "no");
    printf("-------------------------------------\n");
}

void AppStartSynPing() {
    irr::net::CNetSynPing synping;
    s32 ret;
    if(synping.init()) {
        //ret = synping.ping("61.135.169.121", 80);
        ret = synping.ping("221.204.177.67", 80);
        //ret = synping.ping("192.168.1.200", 3306);
        //ret = synping.ping("192.168.1.102", 9981);
        //0: 主机不存在
        //1: 主机存在但没监听指定端口
        //2: 主机存在并监听指定端口
        printf("syn ping ret = %d\n", ret);
    } else {
        printf("syn ping init fail\n");
    }
    printf("-------------------------------------\n");
}

void AppRunTimerWheel() {
    class CTimeAdder : public IRunnable {
    public:
        CTimeAdder(CTimerWheel& it, u32 maxStep, s32 repeat) :
            mIndex(0),
            mMaxRepeat(repeat),
            mMaxStep(maxStep < 5 ? 5 : maxStep),
            mRunning(false),
            mTimer(it) {
            mTimer.setCurrentStep(0x7FFFFFFF - 40);
            //mTimer.setCurrentStep(0x80000000 - 40);
            //mTimer.setCurrentStep(-1);
            mTimer.setInterval(100);
        }
        static void timeout(void* nd) {
            SNode* node = (SNode*) nd;
            s32 idx = AppAtomicDecrementFetch(node->mAdder->getCount());
            if(node->mDeleteFlag) {
                printf("CTimeAdder::timeout>> delete[Adder=%p],[id = %d],[time = %d/%d],[idx = %d]\n",
                    node->mAdder, node->mID, node->mTimeNode.mTimeoutStep,
                    node->mAdder->getTimeWheelStep(), idx);
                delete node;
            } else {
                printf("CTimeAdder::timeout>> repeat[Adder=%p],[id = %d],[time = %d/%d],[idx = %d]\n",
                    node->mAdder, node->mID, node->mTimeNode.mTimeoutStep,
                    node->mAdder->getTimeWheelStep(), idx);
            }
        }
        s32 getTimeWheelStep()const {
            return mTimer.getCurrentStep();
        }
        virtual void run() {
            for(; mRunning;) {
                if(0 == mIndex) {
                    add();
                }
                CThread::sleep(mMaxStep);
            }
        }
        s32* getCount() {
            return &mIndex;
        }
        void start() {
            if(!mRunning) {
                mRunning = true;
                if(mMaxRepeat == 0) {
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
                if(mMaxRepeat == 0) {
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
    printf("@Input repeat times = ");
    u32 repeat = 0;
    scanf("%u", &repeat);
    CTimeAdder tadder1(tmanager.getTimeWheel(), 50, repeat);
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
    irr::IAppLogger::getInstance().addReceiver(
        irr::IAppLogger::ELRT_CONSOLE | irr::IAppLogger::ELRT_FILE_TEXT);
    irr::u32 key = 1;
    while(key) {
        printf("@0 = Exit\n");
        printf("@1 = Net Server\n");
        printf("@2 = Net Client\n");
        printf("@3 = Time Wheel\n");
        printf("@4 = Net Ping\n");
        printf("@5 = Net Syn Ping\n");
        printf("@Input menu id = ");
        scanf("%u", &key);
        switch(key) {
        case 1:
            irr::AppRunEchoServer();
            break;
        case 2:
            irr::AppRunEchoClient();
            break;
        case 3:
            irr::AppRunTimerWheel();
            break;
        case 4:
            irr::AppStartPing();
            break;
        case 5:
            irr::AppStartSynPing();
            break;
        default:break;
        }
        printf("@Task finished\n\n");
    }//while

    printf("@App exit success\n");
    return 0;
}

