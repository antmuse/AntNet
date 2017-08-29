#include "CNetServerSeniorUDP.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <Windows.h>
#include <MSWSock.h>
#endif //APP_PLATFORM_WINDOWS

#include "CNetUtility.h"
#include "IAppTimer.h"
#include "IAppLogger.h"
#include "CNetPacket.h"


///The infomation to inform workers quit.
#define APP_SERVER_EXIT_CODE  -1
///cache count for per client context
#define APP_DEFAULT_CLIENT_CACHE (10)

#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
    namespace net {

        CNetServerSeniorUDP::CNetServerSeniorUDP() : 
    mMaxClientCount(0xFFFFFFFF),
        mRunning(false),
        mReceiver(0),
        mIP(APP_NET_DEFAULT_IP),
        mPort(APP_NET_DEFAULT_PORT),
        mSocket(APP_INVALID_SOCKET){

#if defined(APP_PLATFORM_WINDOWS)
            CNetUtility::loadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)

    }


    CNetServerSeniorUDP::~CNetServerSeniorUDP(){
        stop();
#if defined(APP_PLATFORM_WINDOWS)
        CNetUtility::unloadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
    }


    void CNetServerSeniorUDP::run(){
        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::run", "worker theread start, ID: %d", CThread::getCurrentNativeID());
        s8* buffer = new s8[APP_NET_MAX_BUFFER_LEN]; //buffer for each thread
        sockaddr_in address;
        s32 sizeaddr = sizeof(address);
        s32 ret = 0;
        u32 time;
        for(;mRunning;){
            time = IAppTimer::getTime();
            ret = ::recvfrom(mSocket, buffer,  APP_NET_MAX_BUFFER_LEN, 0, (sockaddr *) (&address),  &sizeaddr);
            if (APP_SOCKET_ERROR == ret) {
                if (!clearError()) {
                    break;
                }
            }else{
                CNetClientID cid(address.sin_addr.S_un.S_addr, address.sin_port);
                core::map<CNetClientID, SClientContextUDP*>::Node* node = mAllContextSocket.find(cid);
                if(node){
                    SClientContextUDP* client = node->getValue();
                    client->mProtocal.import(buffer, ret);
                    if(time >= client->mTime){
                        client->mProtocal.update(time);
                        client->mTime = client->mProtocal.check(client->mTime);
                    }

                    APP_LOG(ELOG_INFO, "CNetServerSeniorUDP::run",  
                        "Step client=[%s:%d=%d]",
                        inet_ntoa(address.sin_addr), 
                        ntohs(address.sin_port),
                        ret);

                    stepReceive(client);
                }else{ //new client
                    SClientContextUDP* newContext = new SClientContextUDP();
                    memcpy(&newContext->mClientAddress, &address, sizeaddr);
                    addClient(newContext);
                    newContext->mProtocal.import(buffer, ret);
                    stepReceive(newContext);
                }
            }

            //update others
            for(core::map<CNetClientID, SClientContextUDP*>::Iterator it= mAllContextSocket.getIterator();
                !it.atEnd(); it++){
                    SClientContextUDP* client = it->getValue();
                    if(time >= client->mTime){
                        client->mProtocal.update(time);
                        client->mTime = client->mProtocal.check(time);
                    }
            }

            CThread::sleep(10);
        }//for

        //mRunning = false;
        delete[] buffer;

        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::run", "worker thread exit, ID: %d", CThread::getCurrentNativeID());
    }


    void CNetServerSeniorUDP::setIP(const c8* ip){
        if(ip){
            mIP = ip;
        }
    }


    bool CNetServerSeniorUDP::start(){
        if(mRunning){
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::start", "server is running currently.");
            return false;
        }


        if (false == initialize())  {
            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::start", "init server fail");
            removeAll();
            return false;
        }

        mThread = new CThread();
        mThread->start(*this);
        mRunning = true;

        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::start", "server start success");
        return true;
    }


    bool CNetServerSeniorUDP::stop() {
        if(!mRunning){
            return false;
        }

        mRunning = false;
        mThread->join();
        removeAllClient();
        removeAll();
        IAppLogger::log(ELOG_INFO,  "CNetServerSeniorUDP::stop", "server stoped success");
        return true;
    }


    bool CNetServerSeniorUDP::initialize() {
        sockaddr_in iServerAddress;
        mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (APP_INVALID_SOCKET == mSocket) {
            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "create listen socket fail: %d", WSAGetLastError());
            return false;
        }

        memset(&iServerAddress, 0, sizeof(iServerAddress));
        iServerAddress.sin_family = AF_INET;
        iServerAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);                      
        //iServerAddress.sin_addr.S_un.S_addr = inet_addr(mIP.c_str());         
        iServerAddress.sin_port = htons(mPort);       
        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::initialize", "IP: [%s:%d]", mIP.c_str(), mPort);

        s32 ret = bind(mSocket, (SOCKADDR*)&iServerAddress, sizeof(iServerAddress));
        if (APP_SOCKET_ERROR == ret) {
            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "bind socket error: %d", WSAGetLastError());
            return false;
        }

        if(!CNetUtility::setNoblock(mSocket)){
            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "socket unblock error: %d", WSAGetLastError());
            return false;
        }

        if(!CNetUtility::setReuseIP(mSocket)){
            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "socket reuse IP error: %d", WSAGetLastError());
            return false;
        }

        if(!CNetUtility::setReusePort(mSocket)){
            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::initialize", "socket reuse port error: %d", WSAGetLastError());
            return false;
        }

        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::initialize", "init ok");
        return true;
    }


    void CNetServerSeniorUDP::removeAll() {
        delete mThread;
        CNetUtility::closeSocket(mSocket);
        mSocket = APP_INVALID_SOCKET;
        mThread = 0;
        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::removeAll", "remove all success");
    }


    bool CNetServerSeniorUDP::stepReceive(SClientContextUDP* iContext){
        sockaddr_in* iClientAddr = &iContext->mClientAddress;
        net::CNetPacket& pack = iContext->mNetPack;
        u8 echoU8 = ENET_NULL;

        s32 ret = iContext->mProtocal.receiveData(pack.getPointer(), pack.getAllocatedSize());
        //s32 ret = iContext->mProtocal.receiveData(pack.getWritePointer(), pack.getWriteSize());

        if(ret >0){
            if(pack.init(ret)) {
                u8 protocal = ENET_NULL;
                do{
                    protocal = pack.readU8();

                    APP_ASSERT(protocal<ENET_COUNT);

                    APP_LOG(ELOG_DEBUG, "CNetServerSeniorUDP::stepReceive",  
                        "received [%s:%d, type=%s]",
                        inet_ntoa(iClientAddr->sin_addr), 
                        ntohs(iClientAddr->sin_port), 
                        AppNetMessageNames[protocal]);


                    switch(protocal){
                    case ENET_DATA:
                        {
                            if(mReceiver){
                                mReceiver->onReceive(pack);
                            }
                            echoU8 = (u8)ENET_HI;
                            break;
                        }
                    default:
                        echoU8 = ENET_HI;
                        break;
                    }//switch
                }while(pack.slideNode());
            } else {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::stepReceive", 
                    "net package init failed: [%u]", pack.getSize());
            }

            pack.clear();
        }//if


        if(ENET_NULL != echoU8){
            net::CNetPacket packSend(1);
            //packSend.clear();
            packSend.add(echoU8);
            packSend.finish();
            iContext->mProtocal.sendData(packSend.getPointer(), packSend.getSize());
        }

        return true;
    }


    void CNetServerSeniorUDP::addClient(SClientContextUDP* iContext){
        mMutex.lock();
        CNetClientID cid(iContext->mClientAddress.sin_addr.S_un.S_addr, iContext->mClientAddress.sin_port);
        mAllContextSocket.set(cid, iContext);
        //iContext->mProtocal.setID(0);
        iContext->mProtocal.setUserPointer(iContext);
        iContext->mProtocal.setSender(this);

        IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::addClient",  
            "incoming client [%d=%s:%d]",
            getClientCount(),
            inet_ntoa(iContext->mClientAddress.sin_addr), 
            ntohs(iContext->mClientAddress.sin_port));

        mMutex.unlock();
    }


    void CNetServerSeniorUDP::removeClient(CNetClientID id){
        mMutex.lock();
        core::map<CNetClientID, SClientContextUDP*>::Node* node = mAllContextSocket.delink(id);
        if(node){
            SClientContextUDP* iContextSocket = node->getValue();
            delete iContextSocket;
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::removeClient",  "client count: %u", getClientCount());
        }
        mMutex.unlock();
    }


    void CNetServerSeniorUDP::removeAllClient(){
        mMutex.lock();
        for(core::map<CNetClientID, SClientContextUDP*>::Iterator it= mAllContextSocket.getIterator();
            !it.atEnd(); it++){
                delete it->getValue();
        }
        mAllContextSocket.clear();
        mMutex.unlock();
    }


    bool CNetServerSeniorUDP::clearError(){
        DWORD ecode = WSAGetLastError(); //GetLastError();
        switch(ecode){
        case 0:
        case WSA_IO_PENDING:
        case WSAEWOULDBLOCK:
        case WSAECONNRESET: //a remote client reset
            return true;
        case WAIT_TIMEOUT:
            //case WSA_WAIT_TIMEOUT:
            {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::clearError", "socket timeout, retry...");
                return true;
            }
        case ERROR_NETNAME_DELETED:
            {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorUDP::clearError",  "client quit abnormally, error: %d", ecode);
                return true;
            }
        default:
            {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorUDP::clearError",  "operating error: %d", ecode);
                return false;
            }
        }//switch
    }



    s32 CNetServerSeniorUDP::sendBuffer(void* iUserPointer, const s8* iData, s32 iLength){
        if(!iUserPointer) return -1;

        SClientContextUDP* iContext = (SClientContextUDP*) iUserPointer;
        s32 ret = 0;
        s32 sent;
        for(;ret<iLength;){
            sent = ::sendto(mSocket, iData+ret, iLength-ret, 0, (sockaddr*)&iContext->mClientAddress, sizeof(iContext->mClientAddress));
            if(sent>0){
                ret+=sent;
            }else{
                if(!clearError()){
                    break;
                }
                CThread::sleep(10);
            }
        }
        APP_LOG(ELOG_INFO, "CNetServerSeniorUDP::sendBuffer", "Sent leftover: [%d - %d]", iLength, ret);
        return ret;
    }

    }// end namespace net
}//namespace irr
#endif //APP_PLATFORM_WINDOWS




#if defined(APP_PLATFORM_LINUX)

#endif //APP_PLATFORM_LINUX



