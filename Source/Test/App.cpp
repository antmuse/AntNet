#include <stdio.h>
#include "SNetAddress.h"
#include "INetManager.h"
#include "IAppLogger.h"
#include "CNetServerAcceptor.h"
#include "CNetClientSeniorTCP.h"
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
    net::SNetAddress addr("221.204.177.67", 9981);
    //net::SNetAddress addr("127.0.0.1", 9981);
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


void AppStartSynPing() {
    irr::net::CNetSynPing synping;
    s32 ret;
    if(synping.init()) {
        ret = synping.ping("61.135.169.121", 80);
        //ret = synping.ping("221.204.177.67", 80);
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
    //irr::IAppLogger::getInstance();
    printf("@1 = Net Server\n");
    printf("@2 = Net Client\n");
    printf("@3 = Net Ping\n");
    printf("@4 = Net Syn Ping\n");
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
        irr::AppStartSynPing();
        break;
    }
    irr::AppQuit();
    return 0;
}

