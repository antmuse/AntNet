#include "HConfig.h"
#include "fast_atof.h"
#include "IAppLogger.h"

#include "IRunnable.h"
#include "CThread.h"
#include "CThreadPool.h"

#include "INetEventer.h"
#include "INetManager.h"
#include "CNetClientUDP.h"
#include "CNetServerSeniorUDP.h"
#include "CNetPacket.h"

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



using namespace irr;



void AppQuit(){
    /*printf("@Please any key to quit\n");
    system("PAUSE");*/
    c8 key = '\0';
    while('*' != key){
        printf("@Please input [*] to quit\n");
        scanf("%c", &key);
    }
}


//for thread test
class CWorker : public IRunnable{
public:
    virtual void run(){
        CThread* td = CThread::getCurrentThread();
        printf("CWorker.start::thread id = %u\n", td->getID());
        CThread::sleep(5000);
        printf("CWorker.stop::thread id = %u\n", td->getID());
    }
};

//for thread test
void AppWorker(void* param){
    CThread* td = CThread::getCurrentThread();
    printf("AppWorker.start::thread id = %u\n", td->getID());
    CThread::sleep(5000);
    printf("AppWorker.stop::thread id = %u\n", td->getID());
}


//for thread test
void AppStartThreads(){
    const u32 max = 3;
    CThread* allthread[max];
    CWorker wk;
    for(u32 i=0; i<max; ++i){
        allthread[i] = new CThread();
        allthread[i]->start(wk);
    }

    for(u32 i=0; i<max; ++i){
        allthread[i]->join();
    }


    CThreadPool pool(max);
    pool.start();
    for(u32 i=0; i<max; ++i){
        pool.start(&wk);
    }
    for(u32 i=0; i<max; ++i){
        pool.start(AppWorker, 0);
    }
    pool.stop();
}


//for net test
class CDefaultNetEventer : public net::INetEventer {
public:
    CDefaultNetEventer(){
    }
    virtual ~CDefaultNetEventer(){
    }
    virtual void onReceive(net::CNetPacket& pack){
        core::array<c8> items;
        pack.seek(0); 
        u8 typeID = pack.readU8();
        switch(typeID){
        case net::ENET_DATA:
            pack.readString(items);
            IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onReceive",  "[%s]", items.pointer());
            break;
        default:  break;
        }//switch
        IAppLogger::log(ELOG_INFO,"CDefaultNetEventer::onReceive", "=======================================");
    }
};


//for net test
void AppStartNetNodes(){
    net::AppGetNetManagerInstance()->start();    CDefaultNetEventer eventer;    //server    net::INetServer* iNetServer = net::AppGetNetManagerInstance()->createServerSeniorUDP();    iNetServer->setNetEventer(&eventer);    //iNetServer->setPort(9012);    iNetServer->start();    //client    net::INetClient* iNet = net::AppGetNetManagerInstance()->addClientUDP();    iNet->setNetEventer(&eventer);    //iNet->setIP("192.168.1.226");    iNet->start();    net::CNetPacket pack(1024);    CThread::sleep(2000);
    for(u32 i=1; i<=20; ++i){        pack.clear();        pack.add(u8(net::ENET_DATA));        core::stringc idstr(i);        idstr.append("------------udp test------------");        pack.add(idstr.c_str(), idstr.size());        pack.finish();        iNet->sendData(pack);    }    printf("KCP segment size = [%u], per packet size = [%u]\n", sizeof(net::CNetProtocal::SKCPSegment), pack.getSize());    CThread::sleep(5000);    iNet->stop();    //quit    AppQuit();    iNetServer->stop();    delete iNetServer;
    net::AppGetNetManagerInstance()->stop();}


int main(int argc, char** argv) {
    s32 key = -1;

    if(argc>=2){
        key = core::strtoul10(argv[1]);
    }else{
        printf("@Input [1] for AntThread lib test.\n");
        printf("@Input [2] for AntNet lib test.\n");
        printf("@Input>");
        scanf("%u", &key);
    }
    printf("@cmd = %d\n", key);

    switch(key){
    case 1:
        AppStartThreads();
        break;
    case 2:
        AppStartNetNodes();
        break;
    default:
        printf("@Unknown cmd----------------------------------\n");
        break;
    };

    AppQuit();
    printf("@Test quit success.\n"); 
    return 0;
}//main
