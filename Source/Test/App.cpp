#include <stdio.h>
#include "CNetAddress.h"
#include "INetManager.h"
#include "IAppLogger.h"
#include "CNetServerAcceptor.h"
#include "CNetServiceTCP.h"
#include "CDefaultNetEventer.h"
#include "CNetPing.h"
#include "CNetSynPing.h"


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
        printf("@Please input [*] to quit\n");
        scanf("%c", &key);
    }
}

void AppStartServer() {
    net::CNetConfig* config = new net::CNetConfig();
    config->mReuse = true;
    config->mMaxPostAccept = 8;
    config->mMaxFetchEvents = 28;
    config->mMaxContext = 200;
    config->mPollTimeout = 1000;
    config->mSessionTimeout = 30000;
    config->mMaxWorkThread = 3;
    config->check();
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
    //evt.setAutoConnect(false);
    accpetor.stop();
    IAppLogger::log(ELOG_INFO, "AppStartServer", "count=%u/%u,size=%u/%u",
        net::CDefaultNetEventer::getSentCount(),
        net::CDefaultNetEventer::getRecvCount(),
        net::CDefaultNetEventer::getSentSize(),
        net::CDefaultNetEventer::getRecvSize());
}

void AppStartClient() {
    net::CNetConfig* config = new net::CNetConfig();
    config->mMaxWorkThread = 3;
    config->mMaxFetchEvents = 128;
    config->mMaxContext = 16;
    config->mPollTimeout = 1000;
    config->mSessionTimeout = 30000;
    config->check();
    config->print();

    const s32 max = 1;
    net::CDefaultNetEventer evt[max];
    u32 session[max];
    net::CNetServiceTCP chub(config);
    config->drop();

    chub.start();
    net::CNetAddress addr("127.0.0.1", 9981);
    s32 i;
    for(i = 0; i < max; ++i) {
        evt[i].setHub(&chub);
        evt[i].setAutoConnect(true);
        session[i] = chub.connect(addr, &evt[i]);
        if(0 == session[i]) {
            break;
        }
        //CThread::sleep(100);
    }

    AppQuit();
    for(i = 0; i < max; ++i) {
        evt[i].setAutoConnect(false);
    }
    chub.stop();
    IAppLogger::log(ELOG_INFO, "AppStartServer", "count=%u/%u,size=%u/%u",
        net::CDefaultNetEventer::getSentCount(),
        net::CDefaultNetEventer::getRecvCount(),
        net::CDefaultNetEventer::getSentSize(),
        net::CDefaultNetEventer::getRecvSize());
}

void AppStartPing() {
    irr::net::CNetPing rpin;
    bool got = rpin.ping("61.135.169.121", 2, 1000);
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

}//namespace irr


int main(int argc, char** argv) {
    irr::IAppLogger::getInstance().addReceiver(irr::IAppLogger::ELRT_CONSOLE);
    irr::u32 key = 1;
    while(key) {
        printf("@0 = Exit\n");
        printf("@1 = Net Server\n");
        printf("@2 = Net Client\n");
        printf("@3 = Net Ping\n");
        printf("@4 = Net Syn Ping\n");
        printf("@Input menu id = ");
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
            irr::AppStartSynPing();
            break;
        default:break;
        }
        printf("@Task finished\n\n");
    }//while

    printf("@App exit success\n");
    return 0;
}

