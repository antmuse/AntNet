#include "CNetClientUDP.h"
#include "CNetUtility.h"
#include "CThread.h"
#include "IAppLogger.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <WS2tcpip.h>
#endif //APP_PLATFORM_WINDOWS

#define APP_MAX_CACHE_NODE	(20)
#define APP_PER_CACHE_SIZE	(1024)


namespace irr {
    namespace net {

        CNetClientUDP::CNetClientUDP() : mSocket(APP_INVALID_SOCKET),
            mRemoteIP(APP_NET_DEFAULT_IP), 
            mMyIP(APP_NET_DEFAULT_IP), 
            mRemotePort(APP_NET_DEFAULT_PORT), 
            mMyPort(0), //any port
            mStatus(ENET_RESET),
            mPacket(APP_PER_CACHE_SIZE),
            mUpdateTime(0),
            mReceiver(0),
            mRunning(false){
#if defined(APP_PLATFORM_WINDOWS)
                CNetUtility::loadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
        }


        CNetClientUDP::~CNetClientUDP() {
            stop();
#if defined(APP_PLATFORM_WINDOWS)
            CNetUtility::unloadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
        }


        s32 CNetClientUDP::sendBuffer(void* iUserPointer, const s8* iData, s32 iLength){
            APP_ASSERT(APP_INVALID_SOCKET!=mSocket);
            s32 ret = 0;
            s32 sent;
            for(;ret<iLength;){
                sent = ::sendto(mSocket, iData+ret, iLength-ret, 0, (sockaddr*)&mRemoteAddress, sizeof(mRemoteAddress));
                if(sent>0){
                    ret+=sent;
                }else{
                    if(!clearError()){
                        break;
                    }
                    CThread::yield();
                }
            }
            APP_LOG(ELOG_INFO, "CNetClientUDP::sendBuffer", "Sent leftover: [%d - %d]", iLength, ret);
            return ret;
        }


        s32 CNetClientUDP::sendData(const c8* iData, s32 iLength){
            switch(mStatus){
            case ENET_HI:
                return 0;
            default:
                if(ENET_WAIT != mStatus){
                    return -1;
                }
            }
            return mProtocal.sendData(iData, iLength);
        }


        s32 CNetClientUDP::sendData(const net::CNetPacket& iData){
            return sendData(iData.getConstPointer(), iData.getSize());
        }


        bool CNetClientUDP::clearError() {
#if defined(APP_PLATFORM_WINDOWS)
            s32 ecode = WSAGetLastError();  // GetLastError();
#elif defined(APP_PLATFORM_LINUX)
            s32 ecode = errno;
#endif

            switch(ecode){
#if defined(APP_PLATFORM_WINDOWS)
            case 0:
            case WSAEWOULDBLOCK: //none data
                return true;
            case WSAECONNRESET: //reset
#elif defined(APP_PLATFORM_LINUX)
            case 0:
            case EGGAIN:
            case EWOULDBLOCK: //none data
                return true;
            case ECONNRESET: //reset
#endif
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::clearError", "server reseted: %d", ecode);
                return false;
            default:
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::clearError", "socket error: %d", ecode);
                return false;
            }//switch

            return false;
        }


        bool CNetClientUDP::update(u32 iTime){
            u32 steptime = iTime-mUpdateTime;
            mUpdateTime = iTime;

            switch(mStatus) {
            case ENET_WAIT:
                step(steptime);
                break;

            case ENET_HI:
                sendTick();
                break;

            case ENET_RESET:
                resetSocket();
                break;

            case ENET_BYE:
                mProtocal.flush();
                sendBye();
                mProtocal.flush();
                mRunning = false;
                if(APP_INVALID_SOCKET != mSocket){
                    if(CNetUtility::closeSocket(mSocket)){
                        IAppLogger::log(ELOG_ERROR, "CNetClientUDP::update", "close socket error");							
                    }
                    mSocket = APP_INVALID_SOCKET;
                }
                return false;

            default:
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::run", "Default? How can you come here?");
                break;

            }//switch

            return true;
        }
        

        void CNetClientUDP::setNetPackage(ENetMessage it, net::CNetPacket& out){
            out.clear();
            out.add((u8)it);
            out.finish();
        }


        void CNetClientUDP::setIP(const c8* ip){
            if(ip){
                mRemoteIP = ip;
            }
        }


        void CNetClientUDP::setPort(u16 port){
            mRemotePort = port;
        }


        bool CNetClientUDP::start(){
            if(mRunning){
                IAppLogger::log(ELOG_INFO, "CNetClientUDP::start", "client is running currently.");
                return false;
            }
            mRunning = true;
            mTickCount = 0;
            mTickTime = 0;
            mProtocal.setSender(this);
            //mStatus = ENET_RESET;
            return true;
        }


        bool CNetClientUDP::stop(){
            if(!mRunning){
                IAppLogger::log(ELOG_INFO, "CNetClientUDP::stop", "client stoped.");
                return false;
            }
            
            mStatus = ENET_BYE; //mRunning = false
            return true;
        }


        void CNetClientUDP::setID(u32 id) {
            mProtocal.setID(id);
        }


        void CNetClientUDP::resetSocket(){
            mTickCount = 0;
            mTickTime = 0;

            if(APP_INVALID_SOCKET != mSocket){
                mStatus = ENET_HI;
                mProtocal.flush();
                IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::resetSocket", "reused previous socket");
                CThread::sleep(5000);
                return;
            }

            memset(&mRemoteAddress, 0, sizeof(mRemoteAddress));

#if defined(APP_PLATFORM_WINDOWS)
            mRemoteAddress.sin_addr.S_un.S_addr=inet_addr(mRemoteIP.c_str());
#elif defined(APP_PLATFORM_LINUX)
            inet_pton(AF_INET,mRemoteIP.c_str(),&mRemoteAddress.sin_addr);
#endif
            mRemoteAddress.sin_family=AF_INET;
            mRemoteAddress.sin_port=htons(mRemotePort);

            mSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(APP_INVALID_SOCKET != mSocket){
                //mReadHandle->clear();
                mStatus = ENET_HI;
                //mProtocal.setID(0);
                bindLocal();
            }else{
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::resetSocket", "create socket fail");
            }
        }


        void CNetClientUDP::sendTick(){
            CNetPacket pack(1);
            setNetPackage(ENET_HI, pack);
            s32 ret = mProtocal.sendData(pack.getPointer(), pack.getSize());
            if(0 == ret){
                mTickTime = 0;
                mStatus = ENET_WAIT;
                IAppLogger::log(ELOG_INFO, "CNetClientUDP::run", "tick %u",mTickCount);
            }
        }


        void CNetClientUDP::sendBye(){
            mTickTime = 0;
            CNetPacket pack;
            setNetPackage(ENET_BYE, pack);
            s32 ret = mProtocal.sendData(pack.getPointer(), pack.getSize());
            if(0 == ret){
                IAppLogger::log(ELOG_INFO, "CNetClientUDP::sendBye", "bye");
            }else{
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::sendBye", "bye error");
            }
        }


        void CNetClientUDP::step(u32 iTime){
            mProtocal.update(mUpdateTime);

            //s32 ret = mProtocal.receiveData(mPacket.getPointer(), mPacket.getAllocatedSize());
            s32 ret = mProtocal.receiveData(mPacket.getWritePointer(), mPacket.getWriteSize());
            if(ret>0){
                if(mPacket.init(ret)){
                    u8 protocal = ENET_NULL;
                    do{
                        protocal = mPacket.readU8();

                        APP_LOG(ELOG_DEBUG, "CNetClientUDP::step",  
                            "received [type=%s]",
                            AppNetMessageNames[protocal]);

                        switch(protocal){
                        case ENET_DATA:
                            {
                                if(mReceiver){
                                    mReceiver->onReceive(mPacket);
                                }
                                break;
                            }
                        default:
                            {
                                //IAppLogger::log(ELOG_ERROR, "CNetClientUDP::step", "unknown protocol: [%u]", mStatus);
                                break;
                            }
                        }//switch
                    }while(mPacket.slideNode());

                }else{
                    IAppLogger::log(ELOG_ERROR, "CNetClientUDP::step", "net package init failed: [%d][%u]", ret, mPacket.getSize());
                }
                //mPacket.clear();
                mTickTime = 0;
                mTickCount = 0;
            }else{
                if(mTickTime >= APP_NET_TICK_TIME){
                    mStatus = ENET_HI;
                    ++mTickCount;
                    if(mTickCount>=APP_NET_TICK_MAX_COUNT){ //relink
                        mTickCount = 0;
                        mTickTime = 0;
                        mStatus = ENET_RESET;
                        IAppLogger::log(ELOG_CRITICAL , "CNetClientUDP::step", "over tick count[%s:%d]", mRemoteIP.c_str(), mRemotePort);
                    }
                }else{
                    mTickTime += iTime;
                }
            }

            /////////////receive////////
            s8 buffer[APP_PER_CACHE_SIZE];
            s32 size = sizeof(mRemoteAddress);
            ret = ::recvfrom(mSocket, buffer, APP_PER_CACHE_SIZE, 0, (sockaddr*)&mRemoteAddress, &size);
            if(ret>0){
                APP_LOG(ELOG_INFO, "CNetClientUDP::step",  
                    "received remote =[%s:%d = %d bytes]",
                    inet_ntoa(mRemoteAddress.sin_addr), 
                    ntohs(mRemoteAddress.sin_port), ret);

                s32 inret = mProtocal.import(buffer, ret);
                if(inret<0){
                    APP_ASSERT(0 && "check protocal");
                    IAppLogger::log(ELOG_ERROR , "CNetClientUDP::step", "protocal import error: [%d]", inret);
                }
            }else if(ret<0){
                if(!clearError()){
                    mStatus = ENET_RESET;
                    IAppLogger::log(ELOG_CRITICAL , "CNetClientUDP::step", "server maybe quit[%s:%d]", mRemoteIP.c_str(), mRemotePort);
                }
            }else if(0==ret){
                mStatus = ENET_RESET;
                IAppLogger::log(ELOG_CRITICAL , "CNetClientUDP::step", "server quit[%s:%d]", mRemoteIP.c_str(), mRemotePort);
            }
        }


        bool CNetClientUDP::bindLocal(){
            if(!CNetUtility::setNoblock(mSocket)){
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::bindLocal", "set socket unblocked fail");
            }

            sockaddr_in localAddress;

            memset(&localAddress, 0, sizeof(localAddress));
            localAddress.sin_family = AF_INET;
            localAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);                      
            localAddress.sin_port = htons(mMyPort);

            s32 ret = bind(mSocket, (sockaddr*)&localAddress, sizeof(localAddress));
            if (APP_SOCKET_ERROR == ret) {
                IAppLogger::log(ELOG_ERROR, "CNetClientUDP::bindLocal", "bind socket error: %d", WSAGetLastError());
                return false;
            }

            s32 len = sizeof(localAddress);
            s8 buf[30];
            getsockname(mSocket, (sockaddr*)&localAddress, &len);  //获取本地信息
            inet_ntop(AF_INET,&localAddress.sin_addr,buf,sizeof(buf));
            mMyIP = buf;
            mMyPort = ntohs(localAddress.sin_port);
            IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::bindLocal", "local: [%s:%d]", mMyIP.c_str(),  mMyPort);

            /*getpeername(mSocket,(struct sockaddr*)&peeraddr,&len);   //获取对端信息
            IAppLogger::log(ELOG_CRITICAL, "CNetClientUDP::bindLocal", "remote: [%s:%d]", 
                inet_ntop(AF_INET,&peeraddr.sin_addr,buf,sizeof(buf)),
                ntohs(peeraddr.sin_port));*/

            return true;
        }


    }// end namespace net
}// end namespace irr

