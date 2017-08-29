#include "CNetClientTCP.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <WS2tcpip.h>
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
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "CThread.h"


#define APP_MAX_CACHE_NODE	(20)
#define APP_PER_CACHE_SIZE	(512)


namespace irr {
    namespace net {

        CNetClientTCP::CNetClientTCP() : mIP(APP_NET_DEFAULT_IP), 
            mMyIP(APP_NET_DEFAULT_IP),
            mPort(APP_NET_DEFAULT_PORT), 
            mMyPort(APP_NET_DEFAULT_PORT), 
            mTickTime(0),
            mTickCount(0),
            mReceiver(0),
            mSocket(APP_INVALID_SOCKET),
            mStatus(ENET_RESET),
            mRunning(false){

#if defined(APP_PLATFORM_WINDOWS)
                CNetUtility::loadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
        }


        CNetClientTCP::~CNetClientTCP(){
            stop();
#if defined(APP_PLATFORM_WINDOWS)
            CNetUtility::unloadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
        }


        bool CNetClientTCP::update(u32 iTime){
            if(!mRunning) {
                APP_LOG(ELOG_DEBUG, "CNetClientTCP::update", "not running");
                return true;
            }

            const u32 iStepTime = iTime-mLastUpdateTime; //unit: ms
            mLastUpdateTime = iTime;

            switch(mStatus) {
            case ENET_WAIT:
                {
                    CNetPacket pack(32);
                    flushDatalist();
                    s32 ret = ::recv(mSocket, pack.getWritePointer(), pack.getWriteSize(), 0);
                    if(ret>0){
                        mTickTime = 0;
                        mTickCount = 0;
                        if(!pack.init(ret)){
                            IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "net package init failed: [%d][%u]", ret, pack.getSize());
                            break;
                        }
                        do{
                            switch(pack.readU8()){
                            case ENET_DATA:
                                {
                                    if(mReceiver){
                                        mReceiver->onReceive(pack);
                                    }
                                    APP_LOG(ELOG_DEBUG, "CNetClientTCP::update", "Got server cmd[%s]", AppNetMessageNames[ENET_DATA]);
                                    break;
                                }
                            case ENET_HI:
                                APP_LOG(ELOG_DEBUG, "CNetClientTCP::update", "Got server cmd[%s]", AppNetMessageNames[ENET_HI]);
                                break;
                            case ENET_WAIT:
                                APP_LOG(ELOG_DEBUG, "CNetClientTCP::update", "Got server cmd[%s]", AppNetMessageNames[ENET_WAIT]);
                                break;
                            default:
                                {
                                    IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "unknown protocol: [%u]", mStatus);
                                    pack.clear();
                                    break;
                                }
                            }//switch
                        }while(pack.slideNode());
                    }else if(0==ret){
                        mStatus = ENET_RESET;
                        IAppLogger::log(ELOG_CRITICAL , "CNetClientTCP::update", "server quit[%s:%d]", mIP.c_str(), mPort);
                    }else{
                        if(!clearError()){
                            mStatus = ENET_RESET;
                            break;
                        }
                        if(mTickTime >= APP_NET_TICK_TIME){
                            //if(mComplete){//完整发包间歇
                            mStatus = ENET_HI;

                            ++mTickCount;
                            if(mTickCount>=APP_NET_TICK_MAX_COUNT){ //relink
                                mTickCount = 0;
                                mTickTime = 0;
                                mStatus = ENET_RESET;
                                IAppLogger::log(ELOG_CRITICAL , "CNetClientTCP::update", "over tick count[%s:%d]", mIP.c_str(), mPort);
                            }
                        }else{
                            mTickTime += iStepTime;
                            //mThread->sleep(iStepTime);
                            //printf(".");
                        }
                    }
                    break;
                }
            case ENET_HI:
                {
                    sendTick();
                    break;
                }
            case ENET_LINK:
                {
                    linkSocket();
                    break;
                }
            case ENET_RESET:
                {
                    resetSocket();
                    break;
                }
            default:
                {
                    IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "Default? How can you come here?");
                    return false;   //tell manager to remove this connection.
                }
            }//switch

            return true;
        }


        void CNetClientTCP::sendTick(){
            CNetPacket pack(1);
            mTickTime = 0;
            setNetPackage(ENET_HI, pack);
            mStatus = ENET_WAIT;
            APP_LOG(ELOG_DEBUG , "CNetClientTCP::sendTick", "tick %u",mTickCount);
            u32 pksize = pack.getSize();
            for(u32 sentsize = 0; sentsize < pksize; ){
                s32 ret = send(mSocket, pack.getPointer()+sentsize, pksize-sentsize, 0);
                if(ret>0){
                    sentsize += ret;
                }else if(!clearError()){
                    mStatus = ENET_RESET;
                    break;
                }else{
                    //mThread->sleep(iStepTime);
                    IAppLogger::log(ELOG_INFO, "CNetClientTCP::sendTick", "Send cache full, ticking...");
                }
            }
        }


        void CNetClientTCP::linkSocket(){
            if(!connect(mSocket,(sockaddr*)&mRemoteAddress,sizeof(mRemoteAddress))){
                IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::linkSocket", "success connected with server: [%s:%d]", mIP.c_str(),mPort);
                mStatus = ENET_HI;
                if(!CNetUtility::setNoblock(mSocket)){
                    IAppLogger::log(ELOG_ERROR, "CNetClientTCP::linkSocket", "set socket unblocked fail");
                }
                struct sockaddr_in localaddr,peeraddr;
                s32 len = sizeof(sockaddr_in);
                s8 buf[30];
                getsockname(mSocket,(struct sockaddr*)&localaddr,&len);  //获取本地信息
                inet_ntop(AF_INET,&localaddr.sin_addr,buf,sizeof(buf));
                mMyIP = buf;
                mMyPort = ntohs(localaddr.sin_port);
                IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::linkSocket", "local: [%s:%d]", mMyIP.c_str(),  mMyPort);

                getpeername(mSocket,(struct sockaddr*)&peeraddr,&len);   //获取对端信息
                IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::linkSocket", "remote: [%s:%d]", 
                    inet_ntop(AF_INET,&peeraddr.sin_addr,buf,sizeof(buf)),
                    ntohs(peeraddr.sin_port));
            }else{
                IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::linkSocket", "can't connect with server: [%s:%d]", mIP.c_str(),mPort);
                //mThread->sleep(5000);
            }
        }


        void CNetClientTCP::resetSocket(){
#if defined(APP_PLATFORM_WINDOWS)
            mRemoteAddress.sin_addr.S_un.S_addr=inet_addr(mIP.c_str());
#elif defined(APP_PLATFORM_LINUX)
            inet_pton(AF_INET,mIP.c_str(),&mRemoteAddress.sin_addr);
#endif
            mRemoteAddress.sin_family=AF_INET;
            mRemoteAddress.sin_port=htons(mPort);

            if(APP_INVALID_SOCKET != mSocket){
                if(CNetUtility::closeSocket(mSocket)){
                    IAppLogger::log(ELOG_ERROR, "CNetClientTCP::resetSocket", "close previous socket error, please restart");							
                }else{
                    IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::resetSocket", "closed previous socket success");							
                }
                //mThread->sleep(1000);
                mStream.clear();
            }

            mSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if(APP_INVALID_SOCKET != mSocket){
                //mReadHandle->clear();
                mTickTime = 0;
                mTickCount = 0;
                mStatus = ENET_LINK;
                IAppLogger::log(ELOG_CRITICAL, "CNetClientTCP::resetSocket", "create socket success");
            }else{
                IAppLogger::log(ELOG_ERROR, "CNetClientTCP::resetSocket", "create socket fail");
            }
        }


        bool CNetClientTCP::clearError() {
#if defined(APP_PLATFORM_WINDOWS)
            s32 ecode = WSAGetLastError();  // GetLastError();
#elif defined(APP_PLATFORM_LINUX)
            s32 ecode = errno;
#endif

            switch(ecode){
#if defined(APP_PLATFORM_WINDOWS)
                //case WSAGAIN:
            case WSAEWOULDBLOCK: //none data
                return true;
            case WSAECONNRESET: //reset
#elif defined(APP_PLATFORM_LINUX)
            case EGGAIN:
            case EWOULDBLOCK: //none data
                return true;
            case ECONNRESET: //reset
#endif
                IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "server reseted: %d", ecode);
                return false;
            default:
                IAppLogger::log(ELOG_ERROR, "CNetClientTCP::update", "socket error: %d", ecode);
                return false;
            }//switch

            return false;
        }


        bool CNetClientTCP::flushDatalist() {
            bool successed = true;
            u32 sentsize;
            u32 packsize;
            s32 ret;						//socket function return value.
            s8* spos;			        //send position in current pack.

            for(; mStream.openRead(&spos, &packsize); ){
                sentsize = 0;

                while(sentsize<packsize){
                    ret = ::send(mSocket, spos, packsize-sentsize, 0);
                    if(ret>0){
                        sentsize += ret;
                        spos +=ret;
                    }else if(!clearError()){
                        mStatus = ENET_RESET;
                        break;
                    }else{
                        mStatus = ENET_WAIT;
                        IAppLogger::log(ELOG_INFO, "CNetClientTCP::flushDatalist", "socket cache full");
                        break;
                    }
                }//while

                if(sentsize>0){
                    mStream.closeRead(sentsize);
                }

                if(sentsize<packsize){//maybe socket cache full
                    successed = false;
                    break;
                }
            }//for

            //APP_LOG(ELOG_INFO, "CNetClientTCP::flushDatalist", "flush [%s]", successed ? "success" : "fail");
            return successed;
        }


        void CNetClientTCP::setNetPackage(ENetMessage it, net::CNetPacket& out){
            out.clear();
            out.add((u8)it);
            out.finish();
        }


        bool CNetClientTCP::start(){
            if(mRunning){
                return false;
            }
            mRunning = true;
            IAppLogger::log(ELOG_INFO, "CNetClientTCP::start", "success");
            return true;
        }


        bool CNetClientTCP::stop(){
            if(!mRunning){
                return false;
            }
            mRunning = false;
            mStream.clear();
            if(APP_INVALID_SOCKET != mSocket){
                CNetUtility::closeSocket(mSocket);
                mSocket = APP_INVALID_SOCKET;
            }
            IAppLogger::log(ELOG_INFO, "CNetClientTCP::stop", "success");
            return true;
        }


        void CNetClientTCP::setIP(const c8* ip){
            if(ip){
                mIP = ip;
            }
        }


        void CNetClientTCP::setPort(u16 port){
            mPort = port;
        }


        s32 CNetClientTCP::sendData(const c8* iData, s32 iLength){
            if(ENET_WAIT != mStatus || !mRunning || !iData){
                IAppLogger::log(ELOG_ERROR, "CNetClientTCP::sendData", "invalid net: %p", this);
                return -1;
            }

            /*if(!mStream.isWritable()){
                APP_LOG(ELOG_INFO, "CNetClientTCP::sendData", "stream pipe full: [%u]", mStream.getBlockCount());
                return 0;
            }*/

            s32 ret = 0;
            u32 step;

            for(;ret<iLength;){
                step =  mStream.write(iData+ret, iLength-ret, false);
                if(step>0){
                    ret+=step;
                }else{
                    CThread::yield();
                }
            }

            return ret;
        }


        s32 CNetClientTCP::sendData(const CNetPacket& pack){
            if(ENET_WAIT != mStatus || !mRunning){
                IAppLogger::log(ELOG_ERROR, "CNetClientTCP::sendData", "invalid net: %p", this);
                return -1;
            }

            /*if(!mStream.isWritable()){
                APP_LOG(ELOG_INFO, "CNetClientTCP::sendData", "stream pipe full: [%u]", mStream.getBlockCount());
                return 0;
            }*/

            const s32 packsize = (s32)pack.getSize();
            const s8* pos = pack.getConstPointer();
            s32 ret = 0;
            u32 step;

             for(;ret<packsize;){
                step =  mStream.write(pos+ret, packsize-ret, false);
                if(step>0){
                    ret+=step;
                }else{
                    CThread::yield();
                }
            }

            return ret;
        }


    }// end namespace net
}// end namespace irr

