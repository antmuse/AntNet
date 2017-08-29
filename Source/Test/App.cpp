#include "INetManager.h"
#include "CNetPacket.h"
#include "CThread.h"
#include "CProcessManager.h"
#include "CDefaultNetEventer.h"


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

    void AppQuit(){
        c8 key = '\0';
        while('*' != key){
            printf("@Please input [*] to quit\n");
            scanf("%c", &key);
        }
    }

    //test net
    void AppStartNetNodes(){
        net::CDefaultNetEventer eventer;
        net::AppGetNetManagerInstance()->start();
        net::INetServer* iNetServer = net::AppGetNetManagerInstance()->createServerSeniorUDP();
        iNetServer->setNetEventer(&eventer);
        //iNetServer->setPort(9012);
        iNetServer->start();

        //client
        net::INetClient* iNet = net::AppGetNetManagerInstance()->addClientUDP();
        iNet->setNetEventer(&eventer);
        iNet->setIP("127.0.0.1");
        iNet->start();
        CThread::sleep(1000);
        net::CNetPacket pack(1024);

        for(u32 i=0; i<200; ++i){
            pack.clear();
            pack.add(u8(net::ENET_DATA));
            core::stringc idstr(i);
            idstr.append("------------udp test------------");
            pack.add(idstr.c_str(), idstr.size());
            pack.finish();
            iNet->sendData(pack);
        }
        //printf("KCP segment size = [%u], per packet size = [%u]\n", sizeof(net::CNetProtocal::SKCPSegment), pack.getSize());
        AppQuit();
        iNet->stop();
        iNetServer->stop();
        delete iNetServer;
        net::AppGetNetManagerInstance()->stop();
    }


    //test process
    void AppStartProcesses(){
        CProcessManager::DProcessParam params;
#if defined(APP_PLATFORM_WINDOWS)
        //params.push_back(io::path("f:\\test.txt"));
        CProcessHandle* proc = CProcessManager::launch("notepad.exe", params);
#elif
        CProcessHandle* proc = CProcessManager::launch("/usr/bin/gnome-calculator", params);
#endif
        if(proc){
            printf("AppStartProcesses success\n");
            proc->wait();
        }else{
            printf("AppStartProcesses failed\n");
        }
        //AppQuit();
    }

}; //namespace irr


int main(int argc, char** argv) {
    //irr::AppStartProcesses();
    irr::AppStartNetNodes();
    printf("@Test quit success.\n"); 
    return 0;
}//main
