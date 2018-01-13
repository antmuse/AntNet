#include "INetManager.h"
#include "CNetPacket.h"
#include "CThread.h"
#include "CProcessManager.h"
#include "CDefaultNetEventer.h"
#include "CNetClientSeniorTCP.h"
#include "CNetServerAcceptor.h"


#if defined(APP_PLATFORM_WINDOWS)
#if defined(APP_OS_64BIT)
#if defined(APP_DEBUG)
#pragma comment (lib, "AntNet-64D.lib")
#pragma comment (lib, "Thread-64D.lib")
#else
#pragma comment (lib, "AntNet-64.lib")
#pragma comment (lib, "Thread-64.lib")
#endif //APP_DEBUG
#else
#if defined(APP_DEBUG)
#pragma comment (lib, "AntNet-32D.lib")
#pragma comment (lib, "Thread-32D.lib")
#else
#pragma comment (lib, "AntNet-32.lib")
#pragma comment (lib, "Thread-32.lib")
#endif //APP_DEBUG
#endif //APP_OS_64BIT

//#pragma comment(lib,"shlwapi.lib")
#pragma comment (lib, "iphlpapi.lib")
#pragma comment (lib, "ws2_32.lib")

#endif  //APP_PLATFORM_WINDOWS


namespace irr {

void AppQuit() {
    c8 key = '\0';
    while('*' != key) {
        printf("@Please input [*] to quit\n");
        scanf("%c", &key);
    }
}

//test net
void AppStartServerSenior() {
    net::CNetServerAcceptor accpetor;
    net::SNetAddress addr("127.0.0.1", 9901);
    accpetor.setLocalAddress(addr);
    accpetor.start();
    AppQuit();
    accpetor.stop();
}

//test net
void AppStartClientSenior() {
    const s32 max = 10;
    net::CDefaultNetEventer evt[max];
    net::INetSession* session[max];
    net::CNetClientSeniorTCP chub;
    chub.start();
    net::SNetAddress addr("127.0.0.1", 9901);
    s32 i;
    for(i = 0; i < max; ++i) {
        evt[i].setHub(&chub);
        session[i] = chub.getSession(&evt[i]);
        if(session[i]) {
            evt[i].setSession(session[i]);
        } else {
            break;
        }
        //CThread::sleep(100);
    }

    AppQuit();
    chub.stop();
}


//test process
void AppStartProcesses() {
    CProcessManager::DProcessParam params;
#if defined(APP_PLATFORM_WINDOWS)
    //params.push_back(io::path("f:\\test.txt"));
    CProcessHandle* proc = CProcessManager::launch("notepad.exe", params);
#elif
    CProcessHandle* proc = CProcessManager::launch("/usr/bin/gnome-calculator", params);
#endif
    if(proc) {
        printf("AppStartProcesses success\n");
        proc->wait();
    } else {
        printf("AppStartProcesses failed\n");
    }
    //AppQuit();
}

}; //namespace irr


int main(int argc, char** argv) {
    int key;
    printf("1. start server\n");
    printf("2. start client\n");
    printf("3. start process\n");
    scanf_s("%d", &key);
    switch(key) {
    case 1:
        irr::AppStartServerSenior();
        break;
    case 2:
        irr::AppStartClientSenior();
        break;
    case 3:
        irr::AppStartProcesses();
        break;
    }
    printf("@Test quit success.\n");
    return 0;
}//main
