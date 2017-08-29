#include "CNetServerNormalTCP.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#elif defined(APP_PLATFORM_LINUX)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include "CNetUtility.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CMutex.h"
#include "CNetPacket.h"


namespace irr {
    namespace net {

        CNetServerThread::CNetServerThread(CNetServerNormalTCP& iHub, netsocket iNode) :
    mStatus(ENET_WAIT),
        mSocket(iNode),
        mHub(iHub),
        mRunning(false){
            mThread = new CThread();
    }


    CNetServerThread::~CNetServerThread(){
        stop();
        delete mThread;
    }


    void CNetServerThread::setSocket(netsocket iNode){
        mSocket = iNode;
    }


    void CNetServerThread::run(){
        CNetPacket pack;
        //CNetPacket packSend;
        s32 iGotSize = 0;
        mStatus = ENET_WAIT;

        if(!CNetUtility::setNoblock(mSocket)){
            IAppLogger::log(ELOG_ERROR, "CNetServerThread::run", "set unblocked error!!!");
        }

        while(mRunning){

            /*struct tcp_info info;
            int len = sizeof(info);
            getsockopt(mNode.mSocket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *) & len);
            if ((info.tcpi_state == TCP_ESTABLISHED)) {
            printf("socket connected\n");
            } else {
            printf("socket disconnected\n");
            }*/

            switch(mStatus){
            case ENET_HI:
                {
                    IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "status--->hi");
                    mStatus = ENET_DATA;
                    break;
                }
            case ENET_WAIT:
                {
                    //IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "status------------------------wait");
                    //printf("*");
                    /*struct timeval tm;
                    s32 sret = select(mNode.mSocket,NULL,NULL,NULL, &tm) ;
                    */
                    iGotSize = recv(mSocket, pack.getWritePointer(), pack.getWriteSize(), 0);
                    if(iGotSize>0){
                        if(!pack.init(iGotSize)){
                            IAppLogger::log(ELOG_ERROR, "CNetServerThread::run", "net package init failed");
                            continue;
                        }
                        //packSend.clear();
                        do{
                            switch(pack.readU8()){
                            case ENET_DATA:
                                {
                                    if(mHub.getNetEventer()){
                                        mHub.getNetEventer()->onReceive(pack);
                                    }
                                    break;
                                }
                            case ENET_WAIT:
                                {
                                    mStatus = ENET_DATA;
                                    IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "received tick");
                                    break;
                                }
                            case ENET_HI:
                                {
                                    mStatus = ENET_HI;
                                    break;
                                }
                            default:
                                {
                                    IAppLogger::log(ELOG_ERROR, "CNetClientTCP::run", "unknown protocol: [%u]", mStatus);
                                    mStatus = ENET_WAIT;
                                    break;
                                }
                            }//switch
                        }while(pack.slideNode());
                        //IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "received client cmd: %d",mStatus);
                    }else if(0==iGotSize){
                        //mStatus = ENET_BYE;
                        IAppLogger::log(ELOG_CRITICAL, "CNetServerThread::run", "client have closed.");
                        mRunning = false;
                    }else{
#if defined(APP_PLATFORM_WINDOWS)
                        s32 ecode = WSAGetLastError();
#elif defined(APP_PLATFORM_LINUX)
                        s32 ecode = errno;
#endif
                        switch(ecode){
#if defined(APP_PLATFORM_WINDOWS)
                        case WSAEWOULDBLOCK: //none data
                            break;
                        case WSAECONNRESET: //reset
#elif defined(APP_PLATFORM_LINUX)
                        case EWOULDBLOCK: //none data
                            break;
                        case ECONNRESET: //reset
#endif
                            {
                                mRunning = false;
                                IAppLogger::log(ELOG_ERROR , "CNetServerThread::run", "received error: server request reset?");
                                break;
                            }
                        default:
                            {
                                IAppLogger::log(ELOG_ERROR, "CNetServerThread::run", "received error: %d", ecode);
                                mRunning = false;
                                break;
                            }
                        }//switch
                        //printf(".");
                        mThread->sleep(50);
                    }
                    break;
                }
            case ENET_DATA:
                {
                    IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "status--->data");
                    iGotSize = 0;
                    u8 echoU8 = ENET_DATA;
                    pack.clear();
                    pack.add(echoU8);
                    pack.finish();
                    s32 iResult = send(mSocket, pack.getPointer(), pack.getSize(), 0);
                    if(APP_SOCKET_ERROR == iResult){
                        IAppLogger::log(ELOG_ERROR, "CNetServerThread::run", "send [ENET_DATA] error!!!");
                    }else{
                        mStatus = ENET_WAIT;
                        IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "request data and wait, status--->data");
                    }
                    break;
                }
            case ENET_BYE:
                {
                    IAppLogger::log(ELOG_CRITICAL, "CNetServerThread::run", "status--->bye");
                    u8 echoU8 = ENET_BYE;
                    pack.clear();
                    pack.add(echoU8);
                    pack.finish();
                    s32 iResult = send(mSocket, pack.getPointer(), pack.getSize(), 0);
                    mRunning = false;
                    break;
                }
            default:
                {
                    IAppLogger::log(ELOG_DEBUG, "CNetServerThread::run", "status--->default");
                    mStatus = ENET_WAIT;
                    break;
                }
            }//switch
            //mThread->sleep(50);
        }//while


        if(APP_INVALID_SOCKET != mSocket){
            if(CNetUtility::closeSocket(mSocket)){
                IAppLogger::log(ELOG_ERROR, "CNetServerThread::run", "socket close error!!!");
            }
            mSocket = APP_INVALID_SOCKET;
        }

        mHub.removeWorker(this);
    }


    bool CNetServerThread::start(){
        if(mRunning){
            return false;
        }

        if(APP_INVALID_SOCKET == mSocket){
            IAppLogger::log(ELOG_ERROR, "CNetServerThread::start", "socket invalid!!!");
            return false;
        }
        mRunning = true;
        mThread->start(*this);
        //IAppLogger::log(ELOG_CRITICAL, "CNetServerThread::start", "Server node runing...");
        return true;
    }


    bool CNetServerThread::stop(){
        if(!mRunning){
            return false;
        }
        mRunning = false;
        mThread->join();
        return true;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //server
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    CNetServerNormalTCP::CNetServerNormalTCP() : mIP(APP_NET_DEFAULT_IP), 
        mPort(APP_NET_DEFAULT_PORT), 
        mMaxClients(50),
        mReceiver(0),
        mSocketServer(APP_INVALID_SOCKET),
        mRunning(false){
            mMutex = new CMutex();
            mThread = new CThread();
#if defined(APP_PLATFORM_WINDOWS)
            CNetUtility::loadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
    }


    CNetServerNormalTCP::~CNetServerNormalTCP(){
        stop();
        delete mThread;
        delete mMutex;
#if defined(APP_PLATFORM_WINDOWS)
        CNetUtility::unloadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
    }


    bool CNetServerNormalTCP::init(){
        sockaddr_in addrServer;
#if defined(APP_PLATFORM_WINDOWS)
        addrServer.sin_addr.S_un.S_addr=htonl(INADDR_ANY);//INADDR_ANY表示任何IP
#elif defined(APP_PLATFORM_LINUX)
        addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
        //inet_pton(AF_INET,"127.0.0.1",&addrServer.sin_addr);
#endif
        addrServer.sin_family=AF_INET;
        addrServer.sin_port=htons(mPort);

        mSocketServer=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(APP_INVALID_SOCKET == mSocketServer){
            IAppLogger::log(ELOG_ERROR, "CNetServerNormalTCP::init", "create socket error!!!");
            return false;
        }

        if(!CNetUtility::setNoblock(mSocketServer)){
            IAppLogger::log(ELOG_ERROR, "CNetServerNormalTCP::init", "set no block error!!!");
            return false;
        }

        s32 ret = bind(mSocketServer,(sockaddr*)&addrServer,sizeof(addrServer));
        if (APP_SOCKET_ERROR == ret) {
            IAppLogger::log(ELOG_ERROR, "CNetServerNormalTCP::init", "bind socket error: %d", ret);
            return false;
        }

        ret = listen(mSocketServer,mMaxClients);
        if (APP_SOCKET_ERROR == ret) {
            IAppLogger::log(ELOG_ERROR, "CNetServerNormalTCP::init", "listen socket error: %d", ret);
            return false;
        }

        return true;
    }


    void CNetServerNormalTCP::run(){
        if(false == init()){
            if(APP_INVALID_SOCKET != mSocketServer){
                CNetUtility::closeSocket(mSocketServer);
            }
            IAppLogger::log(ELOG_ERROR, "CNetServerNormalTCP::run", "init error");
            return;
        }

        sockaddr_in addrClient;
        netsocket node;
#if defined(APP_PLATFORM_WINDOWS)
        s32 len=sizeof(addrClient);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        u32 len=sizeof(addrClient);
#endif

        while(mRunning){
            node = APP_INVALID_SOCKET;
            node = accept(mSocketServer, (sockaddr*)&addrClient, &len);//blocked

            if(node != APP_INVALID_SOCKET){
                //			node.mPort = addrClient.sin_port;
                //#if defined(APP_PLATFORM_WINDOWS)
                //			node.mAddressClient = addrClient.sin_addr.S_un.S_addr;
                //#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
                //			node.mAddressClient = addrClient.sin_addr.s_addr;
                //#endif
                if(mAllClientWait.size()){
                    u32 reused = mAllClientWait.size()-1;
                    mAllClientWait[reused]->setSocket(node);
                    mAllClientWait[reused]->start();
                    mAllClient.push_back(mAllClientWait[reused]);
                    mAllClientWait.erase(reused);
                    IAppLogger::log(ELOG_CRITICAL, "CNetServerNormalTCP::run", "reuse node.");
                }else if(mAllClient.size() < mMaxClients){
                    CNetServerThread* netNode = new CNetServerThread(*this, node);
                    mAllClient.push_back(netNode);
                    netNode->start();
                    IAppLogger::log(ELOG_CRITICAL, "CNetServerNormalTCP::run", "create new worker: %u", mAllClient.size());
                }else{//server is full
                    CNetUtility::closeSocket(node);
                    node = APP_INVALID_SOCKET;
                    IAppLogger::log(ELOG_CRITICAL, "CNetServerNormalTCP::run", "server is full, clients count: [%u]", mAllClient.size());
                }
            }//if

            mThread->sleep(100);
            //IAppLogger::log(ELOG_INFO, "CNetServerNormalTCP::run", "loop++");
        }//while

        IAppLogger::log(ELOG_CRITICAL, "CNetServerNormalTCP::run", "runing node: [%u]", mAllClient.size());
        for(u32 i=0; i<mAllClient.size(); ++i){
            mAllClient[i]->stop();
            //delete mAllClient[i];
        }

        IAppLogger::log(ELOG_CRITICAL, "CNetServerNormalTCP::run", "waiting node: [%u]", mAllClientWait.size());
        for(u32 i=0; i<mAllClientWait.size(); ++i){
            //mAllClientWait[i]->stop();
            delete mAllClientWait[i];
        }

        IAppLogger::log(ELOG_CRITICAL, "CNetServerNormalTCP::run", "server exiting, please wait...");
        mThread->sleep(100);
        for(u32 i=0; i<mAllClient.size(); ++i){
            //mAllClient[i]->stop();
            delete mAllClient[i];
        }

        mAllClient.set_used(0);
        mAllClientWait.set_used(0);

        if(APP_INVALID_SOCKET != mSocketServer){
            CNetUtility::closeSocket(mSocketServer);
            mSocketServer = APP_INVALID_SOCKET;
        }
    }


    bool CNetServerNormalTCP::start(){
        if(mRunning){
            return false;
        }
        mRunning = true;
        mThread->start(*this);
        return true;
    }


    bool CNetServerNormalTCP::stop(){
        if(!mRunning){
            return false;
        }
        mRunning = false;
        mThread->join();
        return true;
    }


    void CNetServerNormalTCP::setIP(const c8* ip){
        if(ip){
            mIP = ip;
        }
    }


    void CNetServerNormalTCP::setPort(u16 port){
        mPort = port;
    }


    void CNetServerNormalTCP::removeWorker(CNetServerThread* it){
        if(mRunning){
            mMutex->lock();
            for(u32 i=0; i<mAllClient.size(); ++i){
                if(it == mAllClient[i]){
                    mAllClient[i] = mAllClient.getLast();
                    mAllClient.set_used(mAllClient.size()-1);  //mAllClient.erase(i);
                    mAllClientWait.push_back(it);					   //don't delete it; reused.
                    IAppLogger::log(ELOG_INFO, "CNetServerNormalTCP::removeWorker", "removed node: [%u]", i);
                    break;
                }
            }
            mMutex->unlock();
        }//if
    }


    }// end namespace net
}// end namespace irr

