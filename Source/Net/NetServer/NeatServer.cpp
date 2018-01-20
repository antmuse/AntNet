#include <stdio.h>
#include "SNetAddress.h"
#include "INetManager.h"
#include "IAppLogger.h"
#include "CNetServerAcceptor.h"
#include "CNetClientSeniorTCP.h"
#include "CDefaultNetEventer.h"

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
        scanf_s("%c", &key);
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

}//namespace irr


int main(int argc, char** argv) {
    irr::IAppLogger::getInstance();
    printf("@1 = Net Server\n");
    printf("@2 = Net Client\n");
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
    }
    irr::AppQuit();
    return 0;
}

