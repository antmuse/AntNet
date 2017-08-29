#include "CNetServerSeniorTCP.h"
#include "CNetUtility.h"
#include "IAppLogger.h"
#include "CNetPacket.h"

// 同时投递的Accept请求的数量(这个要根据实际的情况灵活设置)
#define APP_MAX_POST_ACCEPT 10
///The infomation to inform workers quit.
#define APP_SERVER_EXIT_CODE  0
///cache count for per client context
#define APP_DEFAULT_CLIENT_CACHE (10)

#if defined(APP_PLATFORM_WINDOWS)
namespace irr {
    namespace net {


        void CNetServerSeniorTCP::run(){
            HANDLE iHandleIOCP = getHandleIOCP();
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run", "worker theread start, ID: %d", CThread::getCurrentNativeID());

            OVERLAPPED* iOverlapped = 0;
            SContextIO* iAction = 0;
            SClientContext* iContext = 0;
            DWORD bytesTransfered = 0;
            s32 retValue = 0;

            for(;mRunning;){
                retValue = GetQueuedCompletionStatus(
                    iHandleIOCP,
                    &bytesTransfered,
                    (PULONG_PTR)&iContext,
                    &iOverlapped,
                    INFINITE);

                if (APP_SERVER_EXIT_CODE==(DWORD)iContext) {
                    mRunning = false;
                    break;
                }

                if(!retValue) {  
                    if(!clearError(iContext)) {
                        break;
                    }
                    continue;
                } 

                iAction = APP_GET_VALUE_POINTER(iOverlapped, SContextIO, mOverlapped);  

                if((0 == bytesTransfered) && (EOP_RECEIVE|EOP_SEND) & iAction->mOperationType) {  
                    IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run",  "client [%s:%d] disconnected", inet_ntoa(iContext->mClientAddress.sin_addr), ntohs(iContext->mClientAddress.sin_port) );
                    removeClient(iContext);
                    continue;  
                }

                iAction->mBytes = bytesTransfered;
                bool ret = true;
                switch(iAction->mOperationType) {  
                case EOP_RECEIVE:
                    {
                        ret = stepReceive(iContext, iAction);	
                        break;
                    }
                case EOP_SEND:
                    {
                        ret = stepSend(iContext, iAction);	
                        break;
                    }
                case EOP_ACCPET:
                    { 
                        ret = stepAccpet(iContext, iAction);						
                        break;
                    }
                default:
                    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::run", "unknown operation type");
                    break;
                } //switch

                if(!ret){
                    if(!clearError(iContext)) {
                        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::run", "Can't deal with step error, removed a client.");
                        removeClient(iContext);
                    }
                }

            }//while

            onThreadQuit();
            //mRunning = false;
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run", "worker thread exit, ID: %d", CThread::getCurrentNativeID());
        }





        //////////////////////////////////////CNetServerSeniorTCP////////////////////////////////////////////////////
        CNetServerSeniorTCP::CNetServerSeniorTCP() : mThreadCount(APP_NET_DEFAULT_SERVER_THREAD),
            mMaxClientCount(0xFFFFFFFF),
            mRunning(false),
            mHandleIOCP(INVALID_HANDLE_VALUE),
            mReceiver(0),
            mIP(APP_NET_DEFAULT_IP),
            mPort(APP_NET_DEFAULT_PORT),
            mLPFNAcceptEx(0),
            mListenContext(0){
#if defined(APP_PLATFORM_WINDOWS)
                CNetUtility::loadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
                mAllContextSocket.reallocate(1000);
        }


        CNetServerSeniorTCP::~CNetServerSeniorTCP(){
            stop();
#if defined(APP_PLATFORM_WINDOWS)
            CNetUtility::unloadSocketLib();
#endif //#if defined(APP_PLATFORM_WINDOWS)
        }


        void CNetServerSeniorTCP::setIP(const c8* ip){
            if(ip){
                mIP = ip;
            }
        }


        bool CNetServerSeniorTCP::start(){
            if(mRunning){
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::start", "server is running currently.");
                return false;
            }
            //mHandleShutdown = CreateEvent(0, TRUE, FALSE, 0);
            mRunning = true;

            if (false == initialize())  {
                mRunning = false;
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::start",  "init server fail");
                return false;
            }

            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::start", "server start success");
            return true;
        }


        bool CNetServerSeniorTCP::stop() {
            if(!mRunning){
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stop", "server had stoped.");
                return false;
            }

            mRunning = false;

            APP_ASSERT(mListenContext);

            //SetEvent(mHandleShutdown);

            IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "inform all work threads quit, wait...");
            for(u32 i=0; i<mAllWorker.size(); ++i){
                PostQueuedCompletionStatus(mHandleIOCP, 0, (DWORD)APP_SERVER_EXIT_CODE, 0);
                CThread::sleep(10);
            }

            //WaitForMultipleObjects(cpuCoreCount, mHandleWorkerThreads, TRUE, INFINITE);
            for (u32 i = 0; i < mAllWorker.size(); ++i) {
                mAllWorker[i]->join();
            }

            IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "all worker quit success, workers count: [%d]", mAllWorker.size());

            removeAllClient();
            removeAll();
            IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "server stoped success");
            return true;
        }


        bool CNetServerSeniorTCP::initialize() {
            mHandleIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

            if (!mHandleIOCP) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initialize", "create IOCP fail: %d", WSAGetLastError());
                return false;
            }

            if(!initializeSocket()) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initialize", "init listen socket fail");
                removeAll();
                return false;
            }

            //user should according to cpu cores to create worker threads
            mAllWorker.reallocate(mThreadCount,false);
            CThread* wk;
            for (u32 i = 0; i < mThreadCount; ++i) {
                wk = new CThread();
                wk->start(*this);
                mAllWorker.push_back(wk);
                CThread::sleep(20);
            }

            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::initialize", "created worker thread, total: [%d]", mThreadCount);
            return true;
        }


        bool CNetServerSeniorTCP::initializeSocket(){
            // AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
            GUID GuidAcceptEx = WSAID_ACCEPTEX;  
            GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS; 
            SOCKADDR_IN iServerAddress;

            mListenContext = new SClientContext();

            // 重叠IO必须用WSASocket来建立Socket
            mListenContext->mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
            if (APP_INVALID_SOCKET == mListenContext->mSocket) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket", "create listen socket fail: %d", WSAGetLastError());
                return false;
            }

            //if(0!=CNetUtility::setReceiveCache(mListenContext->mSocket, 0xFFFF)){
            //    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket", "set socket receive cache fail: [%d]", WSAGetLastError());
            //}

            if(!bindWithIOCP(mListenContext)) {
                delete mListenContext;
                mListenContext = 0;
                //CNetUtility::closeSocket(mListenContext->mSocket);
                //mListenContext->mSocket=APP_INVALID_SOCKET;
                return false;
            }

            ZeroMemory((c8*)&iServerAddress, sizeof(iServerAddress));
            iServerAddress.sin_family = AF_INET;
            iServerAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);                      
            //iServerAddress.sin_addr.S_un.S_addr = inet_addr(mIP.c_str());         
            iServerAddress.sin_port = htons(mPort);       
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::initializeSocket", "IP: [%s:%d]", mIP.c_str(), mPort);

            s32 ret = bind(mListenContext->mSocket, (SOCKADDR*)&iServerAddress, sizeof(iServerAddress));
            if (APP_SOCKET_ERROR == ret) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket", "bind socket error: %d", WSAGetLastError());
                return false;
            }

            ret = listen(mListenContext->mSocket,SOMAXCONN);
            if (APP_SOCKET_ERROR == ret) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket", "listen socket error: %d", WSAGetLastError());
                return false;
            }

            // 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
            // 所以需要额外获取AcceptEx函数指针
            DWORD dwBytes = 0;  
            if(APP_SOCKET_ERROR == WSAIoctl(
                mListenContext->mSocket, 
                SIO_GET_EXTENSION_FUNCTION_POINTER, 
                &GuidAcceptEx, 
                sizeof(GuidAcceptEx), 
                &mLPFNAcceptEx, 
                sizeof(mLPFNAcceptEx), 
                &dwBytes, 
                0, 
                0))   {
                    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket", "WSAIoctl error: [%d]", WSAGetLastError()); 
                    removeAll();
                    return false; 
            }

            // 获取GetAcceptExSockAddrs函数指针，也是同理
            if(APP_SOCKET_ERROR == WSAIoctl(
                mListenContext->mSocket, 
                SIO_GET_EXTENSION_FUNCTION_POINTER, 
                &GuidGetAcceptExSockAddrs,
                sizeof(GuidGetAcceptExSockAddrs), 
                &mLPFNGetAcceptExSockAddress, 
                sizeof(mLPFNGetAcceptExSockAddress),   
                &dwBytes, 
                0, 
                0))  {  
                    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket", "WSAIoctl GuidGetAcceptExSockAddrs error: [%d]", WSAGetLastError());  
                    removeAll();
                    return false; 
            }  


            // 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
            const u32 addsize = sizeof(SOCKADDR_IN)+16;
            const u32 each = sizeof(netsocket) + 2*addsize;	
            core::array<SContextIO>& allaccpet = mListenContext->mAllContextIO;
            allaccpet.reallocate(APP_MAX_POST_ACCEPT);
            mListenContext->mNetPack.reallocate(each*APP_MAX_POST_ACCEPT);
            memset(mListenContext->mNetPack.getPointer(), 0, mListenContext->mNetPack.getAllocatedSize());
            for(s32 id=0; id<APP_MAX_POST_ACCEPT; ++id) {
                allaccpet.set_used(id + 1);
                allaccpet[id].init();
                allaccpet[id].mID = id;
                allaccpet[id].mOperationType = EOP_ACCPET;
                if(!postAccept(mListenContext, &allaccpet[id])) {
                    delete mListenContext;
                    mListenContext = 0;
                    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initializeSocket",  "post accpet fail, id: [%d]", id);
                    return false;
                }
            }
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::initializeSocket",  "post AcceptEx success, total: %d", APP_MAX_POST_ACCEPT );

            return true;
        }


        void CNetServerSeniorTCP::removeAll() {
            for(u32 i=0; i<mAllWorker.size(); ++i ) {
                delete mAllWorker[i];
            }
            mAllWorker.clear();
            if(INVALID_HANDLE_VALUE!=mHandleIOCP)	{
                CloseHandle(mHandleIOCP);
            }
            delete mListenContext;
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::removeAll", "remove all success");
        }


        bool CNetServerSeniorTCP::postAccept(SClientContext* iContext, SContextIO* iAction) {
            const u32 addsize = sizeof(SOCKADDR_IN)+16;
            const u32 each = sizeof(netsocket) + 2*addsize;	
            c8* const startPos = iContext->mNetPack.getPointer() + iAction->mID*each;
            netsocket* sock = (netsocket*)(startPos);
            *sock  = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);  
            if(APP_INVALID_SOCKET== *sock) {
                return false;  
            }

            if(FALSE == mLPFNAcceptEx(iContext->mSocket, 
                *sock,
                (void*)(startPos+sizeof(netsocket)), 
                0, //(addsize*2),
                addsize, 
                addsize, 
                &iAction->mBytes, 
                &iAction->mOverlapped)) {
                    if(WSA_IO_PENDING != WSAGetLastError()) {
                        CNetUtility::closeSocket(*sock);
                        IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::postAccept", "post accpet[%d] fail: %d", iAction->mID, WSAGetLastError());  
                        return false;  
                    }  
            }

            return true;
        }


        bool CNetServerSeniorTCP::stepAccpet(SClientContext* iContext, SContextIO* iAction) {
            const u32 addsize = sizeof(SOCKADDR_IN)+16;
            const u32 each = sizeof(netsocket) + 2*addsize;	
            c8* startPos = iContext->mNetPack.getPointer() + iAction->mID*each + sizeof(netsocket);
            netsocket sock = *(netsocket*) (startPos-sizeof(netsocket));

            SOCKADDR_IN* iClientAddr = 0;
            SOCKADDR_IN* iLocalAddr = 0;
            s32 remoteLen = sizeof(SOCKADDR_IN);
            s32 localLen = sizeof(SOCKADDR_IN);  

            // 1. 首先取得连入客户端的地址信息
            // 这个mLPFNGetAcceptExSockAddress
            // 不但可以取得客户端和本地端的地址信息，还能顺便取出客户端发来的第一组数据
            //Here just accpet client, does not read the first message of incoming client.
            mLPFNGetAcceptExSockAddress(startPos, 0/*2*addsize*/,  
                addsize, addsize, 
                (LPSOCKADDR*)&iLocalAddr, &localLen, 
                (LPSOCKADDR*)&iClientAddr, &remoteLen);

            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stepAccpet",  
                "slot[%u], incoming client=[%s:%d]",
                iAction->mID,
                inet_ntoa(iClientAddr->sin_addr), 
                ntohs(iClientAddr->sin_port));

            // 2. copy
            SClientContext* newContext = new SClientContext();
            newContext->mSocket = sock;
            memcpy(&(newContext->mClientAddress), iClientAddr, sizeof(iClientAddr));
            newContext->setCache(APP_DEFAULT_CLIENT_CACHE);

            if(!bindWithIOCP(newContext)) {
                delete newContext;
                return false;
            }

            // 3. 
            if(!postReceive(newContext, &newContext->mReceiver)) {
                delete newContext;
                return false;
            }

            // 4. 
            addClient(newContext);

            //6.
            return postAccept(iContext, iAction);
        }


        bool CNetServerSeniorTCP::postSend(SClientContext* iContext, SContextIO* iAction, bool userPost/* = true*/) {
            if(userPost && iContext->mSendPosted) return true;

            SQueueRingFreelock* current = iContext->mReadHandle;

            if(!current->isReadable()) {
#if defined(APP_DEBUG)
                APP_LOG(ELOG_INFO, "CNetServerSeniorTCP::postSend", "can't reading now: [%d]", iContext->mCacheCount);
#endif
                iContext->mSendPosted = false;
                return true;
            }

            iContext->mSendPosted = true;

            WSABUF* iBuffer   = &(iAction->mBuffer);
            s32 pos = current->mCurrentPosition;
            s32 packsize = current->mUserDataSize;
            iBuffer->buf = current->mData + pos;
            iBuffer->len = packsize-pos;
            iAction->mBytes = 0;
            s32 ret = WSASend(iContext->mSocket, iBuffer, 1, &(iAction->mBytes), iAction->mFlags, &iAction->mOverlapped, 0);
            if(APP_SOCKET_ERROR == ret){
                if(!clearError(iContext)){
                    iContext->mSendPosted = false;
                    return false;
                }
            }
            //APP_LOG(ELOG_INFO, "CNetServerSeniorTCP::postSend", "flush finished");
            return true;
        }


        bool CNetServerSeniorTCP::stepSend(SClientContext* iContext, SContextIO* iAction){
            SQueueRingFreelock* current = iContext->mReadHandle;
            s32 pos = current->mCurrentPosition;
            const s32 packsize = current->mUserDataSize;

            APP_ASSERT(iAction->mBuffer.len == iAction->mBytes);

            if(iAction->mBytes>0){
                pos += iAction->mBytes;
                if(pos == packsize){
                    iContext->mPackComplete = true;
                    iContext->mReadHandle = current->getNext();
                    current->setWritable();
#if defined(APP_DEBUG)
                    iContext->mCacheCount--;
#endif
                }else{
                    iContext->mPackComplete = false;
                    current->mCurrentPosition = pos;
                }
            }else if(!clearError(iContext)) {
                return false;
            }

            return postSend(iContext, iAction, false);
        }


        bool CNetServerSeniorTCP::postReceive(SClientContext* iContext, SContextIO* iAction){
            //if(iContext->mReceivePosted) return true;
            //iContext->mReceivePosted = true;

            WSABUF* wBuffer   = &(iAction->mBuffer);
            OVERLAPPED* iOL = &(iAction->mOverlapped);
            wBuffer->buf = iContext->mNetPack.getWritePointer();
            wBuffer->len = iContext->mNetPack.getWriteSize();

            APP_ASSERT(wBuffer->len>0);

            s32 ret = WSARecv(iContext->mSocket, wBuffer, 1, &(iAction->mBytes), &(iAction->mFlags), iOL, 0);

            if ((APP_SOCKET_ERROR == ret) && !clearError(iContext)) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::postReceive", "post recieve failed");
                return false;
            }
            //APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::postReceive", "post WSARecv success [%lu]", wBuffer->len);
            return true;
        }


        bool CNetServerSeniorTCP::stepReceive(SClientContext* iContext, SContextIO* iAction){
            SOCKADDR_IN* iClientAddr = &iContext->mClientAddress;
            net::CNetPacket packSend(5);
            net::CNetPacket& pack = iContext->mNetPack;
            u8 echoU8 = ENET_NULL;

            if(pack.init(u32(iAction->mBytes))) {
                u8 protocal = ENET_NULL;
                do{
                    protocal = pack.readU8();

                    APP_ASSERT(protocal<ENET_COUNT);

                    APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::stepReceive",  
                        "received=[%s:%d] info=[data]",
                        inet_ntoa(iClientAddr->sin_addr), 
                        ntohs(iClientAddr->sin_port), 
                        AppNetMessageNames[protocal]);


                    switch(protocal){
                    case ENET_DATA:
                        {
                            if(mReceiver){
                                mReceiver->onReceive(pack);
                            }
                            echoU8 = (u8)ENET_DATA;
                            break;
                        }
                    case ENET_WAIT:
                        {
                            echoU8 = (u8)ENET_HI;
                            break;
                        }
                    case ENET_HI:
                        {
                            echoU8 = (u8)ENET_WAIT;
                            break;
                        }
                    default:
                        break;
                    }//switch
                }while(pack.slideNode());
            } else {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stepReceive", 
                    "net package init failed: [%ld][%u]", iAction->mBytes, pack.getSize());
            }

            if(ENET_NULL != echoU8){
                packSend.clear();
                packSend.add(echoU8);
                packSend.finish();
                if(addToSendList(iContext, packSend)){
                    postSend(iContext, &iContext->mSender);
                }
            }

            return postReceive(iContext, iAction);
        }


        bool CNetServerSeniorTCP::addToSendList(SClientContext* iContext, CNetPacket& pack){
            if(!iContext) return false;

            SQueueRingFreelock* pk = iContext->mWriteHandle;
            if(pk->isWritable()){
                if(pack.getSize() <= pk->mSize){
                    memcpy(pk->mData, pack.getPointer(), pack.getSize());
                    pk->mUserDataSize = pack.getSize();
                    pk->mCurrentPosition = 0;
                    pk->setReadable();
                    iContext->mWriteHandle = iContext->mWriteHandle->getNext();
#if defined(APP_DEBUG)
                    iContext->mCacheCount++;
#endif
                }else{
                    APP_LOG(ELOG_INFO, "CNetServerSeniorTCP::addToSendList", "cache is not enough, data drop.");
                    return false;
                }
            }else{
                APP_LOG(ELOG_INFO, "CNetServerSeniorTCP::addToSendList", "can't write now: [%d]", iContext->mCacheCount);
                return false;
            }
            return true;
        }


        bool CNetServerSeniorTCP::bindWithIOCP(SClientContext* iContext){
            HANDLE hTemp = CreateIoCompletionPort((HANDLE)iContext->mSocket, mHandleIOCP, (DWORD)iContext, 0);
            if (!hTemp) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::bindWithIOCP", "CreateIoCompletionPort error: [%d]",GetLastError());
                return false;
            }
            return true;
        }


        void CNetServerSeniorTCP::addClient(SClientContext* iContextSocket){
            mMutex.lock();
            mAllContextSocket.push_back(iContextSocket);
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::addClient",  "client count: %u", getClientCount());
            mMutex.unlock();
        }


        void CNetServerSeniorTCP::removeClient(SClientContext* iContextSocket){
            mMutex.lock();
            for(u32 i=0; i<mAllContextSocket.size(); ++i){
                if(iContextSocket==mAllContextSocket[i]){
                    delete iContextSocket;
                    //quickly removed here. don't use the mAllContextSocket.erase(i);
                    mAllContextSocket[i] = mAllContextSocket.getLast();
                    mAllContextSocket.set_used(mAllContextSocket.size()-1);
                    IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::removeClient",  "client count: %u", getClientCount());
                    break;
                }
            }
            mMutex.unlock();
        }


        void CNetServerSeniorTCP::removeAllClient(){
            mMutex.lock();
            for(u32 i=0; i<mAllContextSocket.size(); ++i ){
                delete mAllContextSocket[i];
            }
            mAllContextSocket.clear();
            mMutex.unlock();
        }


        bool CNetServerSeniorTCP::isSocketAlive(netsocket s){
            s32 nByteSent = send(s,"",0,0);
            return (APP_SOCKET_ERROR == nByteSent) ? false : true;
        }


        bool CNetServerSeniorTCP::clearError(SClientContext* pContext){
            DWORD ecode = WSAGetLastError(); //GetLastError();
            switch(ecode){
            case 0:
            case WSA_IO_PENDING:
                return true;
            case WAIT_TIMEOUT:
                //case WSA_WAIT_TIMEOUT:
                {
                    if(!isSocketAlive(pContext->mSocket)) {
                        IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::clearError",  "client quit abnormally, error: %d", ecode);
                        removeClient(pContext);
                        return true;
                    } else {
                        IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::clearError", "socket timeout, retry...");
                        return true;
                    }
                }
            case ERROR_NETNAME_DELETED:
                {
                    IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::clearError",  "client quit abnormally, error: %d", ecode);
                    removeClient( pContext );
                    return true;
                }
            default:
                {
                    IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::clearError",  "IOCP operating error: %d", ecode);
                    return false;
                }
            }//switch
        }


        void CNetServerSeniorTCP::onThreadQuit(){
            mMutex.lock();
            mThreadCount--;
            mMutex.unlock();
        }

    }// end namespace net
}//namespace irr
#endif //APP_PLATFORM_WINDOWS




#if defined(APP_PLATFORM_LINUX)
#include <errno.h>

namespace irr {
    namespace net {

        //////////////////////////////////////CNetServerSeniorTCP////////////////////////////////////////////////////
        CNetServerSeniorTCP::CNetServerSeniorTCP(CNetServerSeniorTCP& iServer) : mRunning(false),
            mServer(iServer) {
        }


        CNetServerSeniorTCP::~CNetServerSeniorTCP() {
            stop();
        }


        void CNetServerSeniorTCP::run() {
            //mRunning = true;

            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run", "worker theread start, ID: %d",mThread.getID());
            s32 ret = 0;
            s32 epollFD = mServer.getHandle();
            epoll_event allEvent[APP_MAX_POST_ACCEPT];
            SClientContext* iContextSocket = 0;
            SContextIO* sockIO = 0;
            s32 emask = 0;

            for(; mRunning;) {
                ret = epoll_wait(epollFD,allEvent,APP_MAX_POST_ACCEPT,100);
                if(ret>=0){
                    for(s32 n = 0; n < ret; ++n) {
                        sockIO = (SContextIO*)allEvent[n].data.ptr;
                        iContextSocket = sockIO->mContext;
                        emask = allEvent[n].events;
                        if(emask & EPOLLIN) {
                            if(EOP_ACCPET == sockIO->mOperationType) {
                                mServer.stepAccpet(iContextSocket, sockIO);
                            } else {
                                mServer.stepReceive(iContextSocket,sockIO);
                            }
                        } else if(emask & EPOLLOUT) {
                            if(EOP_ACCPET == sockIO->mOperationType) {
                                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run", "listen socket writable: %d", iContextSocket->mSocket);
                                mRunning = false;
                            } else {
                                mServer.stepSend(iContextSocket,sockIO);
                            }
                        } else if(emask & EPOLLERR) {
                            mServer.clearError(iContextSocket);
                        }else if(emask & EPOLLHUP){
                            IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::run", "socket closed: %d", mThread.getID());
                        }
                    }
                }else{
                    //error
                }
            }//for

            mServer.removeWorker(this);
            //mRunning = false;
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::run", "worker thread exit, ID: %d", mThread.getID());
        }


        void CNetServerSeniorTCP::start() {
            if(!mRunning) {
                mRunning = true;
                mThread.start(*this);
            }
        }


        void CNetServerSeniorTCP::stop() {
            if(mRunning) {
                mRunning = false;
                mThread.join();
            }
        }


        //////////////////////////////////////CNetServerSeniorTCP////////////////////////////////////////////////////
        CNetServerSeniorTCP::CNetServerSeniorTCP() : mMaxWorkerCount(APP_NET_MAX_SERVER_THREAD),
            mMaxClientCount(0x7FFFFFFF),
            mRunning(false),
            mReceiver(0),
            mListenContext(0),
            mEpollFD(-1),
            mIP(APP_NET_DEFAULT_IP),
            mPort(APP_NET_DEFAULT_PORT) {

        }


        CNetServerSeniorTCP::~CNetServerSeniorTCP() {
            stop();
        }


        bool CNetServerSeniorTCP::initialize() {
            s32 ret;
            mListenContext = new SClientContext;
            mListenContext->mSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            if (mListenContext->mSocket == -1) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initialize", "init listen socket fail: %d", errno);
                return false;
            }

            if(!CNetUtility::setNoblock(mListenContext->mSocket)) {
                return false;
            }

            //set port resued
            if(!CNetUtility::setReuse(mListenContext->mSocket)) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initialize", "set port resuse fail: %d", errno);
                return false;
            }

            struct sockaddr_in servaddr;
            bzero(&servaddr,sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            //inet_pton(AF_INET,mIP.c_str(),&servaddr.sin_addr);
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            servaddr.sin_port = htons(mPort);
            ret = bind(mListenContext->mSocket,(struct sockaddr*)&servaddr,sizeof(servaddr));
            if (ret == -1) {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::initialize", "bind socket fail: %d", errno);
                return false;
            }

            mEpollFD = epoll_create(APP_MAX_POST_ACCEPT);

            ret = listen(mListenContext->mSocket,mMaxClientCount);
            if (ret == -1) {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::initialize", "listen socket fail: %d", errno);
                return false;
            }


            SContextIO* it = mListenContext->getNewContexIO();
            if(!postAccept(it)) {
                mListenContext->removeContextIO(it);
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::initialize", "post accept fail: %d", errno);
            }


            u32 cpuCoreCount = 2 + APP_MAX_THREAD_PER_CPU_CORE * IEngine::getInstance().getCollector().getCPUCoreCount();
            cpuCoreCount = core::min_<u32>(cpuCoreCount,mMaxWorkerCount);
            mAllWorker.reallocate(cpuCoreCount,false);
            for (u32 i = 0; i < cpuCoreCount; ++i) {
                mAllWorker.push_back(new CNetServerSeniorTCP(*this));
                mAllWorker[i]->start();
                CThread::sleep(20);
            }

            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::initialize", "created worker thread, total: %d", cpuCoreCount );
            return true;
        }


        void CNetServerSeniorTCP::removeAll() {
            for(u32 i=0; i<mAllWorker.size(); ++i ) {
                delete mAllWorker[i];
            }
            mAllWorker.clear();
            if(-1!=mEpollFD)	{
                close(mEpollFD);
                mEpollFD = -1;
            }
            //registeEvent(mListenContext->mAllIOContext[0], EPOLL_CTL_DEL, 0xffffffff);
            delete mListenContext;
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::removeAll", "remove all success");
        }


        void CNetServerSeniorTCP::removeWorker(CNetServerSeniorTCP* it) {
            mMutex.lock();
            for(u32 i=0; i<mAllWorker.size(); ++i) {
                if(mAllWorker[i] == it) {
                    delete it;
                    mAllWorker.erase(i);
                    break;
                }
            }
            mMutex.unlock();
        }


        bool CNetServerSeniorTCP::start() {
            if(mRunning) {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::start", "server is running currently.");
                return false;
            }
            if (initialize())  {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::start", "init server success");
            }  else  {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::start", ("init server fail"));
                return false;
            }
            mRunning = true;
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::start", "server start success");
            return true;
        }


        bool CNetServerSeniorTCP::stop() {
            if(!mRunning) {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stop", "server had stoped.");
                return false;
            }

            mRunning = false;
            //IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "server stoping...");

            if(!mListenContext || APP_INVALID_SOCKET == mListenContext->mSocket) {
                IAppLogger::log(ELOG_WARNING,  "CNetServerSeniorTCP::stop", "are you sure the server init successed ?");
            }

            //registeEvent(mListenContext->mAllIOContext[0], EPOLL_CTL_MOD, EPOLLOUT | EPOLLERR | EPOLLHUP); //note: NO EPOLLET here.

            IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "inform all work threads quit, wait...");
            while(mAllWorker.size()) {
                mAllWorker[0]->stop();
                CThread::sleep(10);
            }

            //APP_ASSERT(!mAllWorker.size());

            IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "all work threads quit success, workers count: %d", mAllWorker.size());

            removeAllClient();
            removeAll();
            IAppLogger::log(ELOG_INFO,  "CNetServerSeniorTCP::stop", "server stoped success");
            return true;
        }


        bool CNetServerSeniorTCP::sendData(const net::CNetPacket& iData) {
            return true;
        }


        bool CNetServerSeniorTCP::sendData(const c8* iData, s32 iLength) {
            return true;
        }


        void CNetServerSeniorTCP::setIP(const c8* ip) {
            if(ip) {
                mIP = ip;
            }
        }


        bool CNetServerSeniorTCP::registeEvent(SContextIO* iContextIO, s32 action, u32 mask) {
            epoll_event ev;
            ev.events = mask;
            ev.data.ptr = iContextIO;
            //action = EPOLL_CTL_ADD, EPOLL_CTL_DEL, EPOLL_CTL_MOD
            s32 ret = epoll_ctl(mEpollFD,action,iContextIO->mContext->mSocket,&ev);
            if(ret) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::registeEvent","fail: %d",errno);
            }
            return 0==ret;
        }


        void CNetServerSeniorTCP::addClient(SClientContext* iContextSocket) {
            mMutex.lock();
            mAllContextSocket.push_back(iContextSocket);
            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::addClient",  "client count: %u", getClientCount());
            mMutex.unlock();
        }


        void CNetServerSeniorTCP::removeClient(SClientContext* iContextSocket) {
            mMutex.lock();
            for(u32 i=0; i<mAllContextSocket.size(); ++i) {
                if(iContextSocket==mAllContextSocket[i]) {
                    delete iContextSocket;
                    //quickly removed here. don't use the mAllContextSocket.erase(i);
                    mAllContextSocket[i] = mAllContextSocket.getLast();
                    mAllContextSocket.set_used(mAllContextSocket.size()-1);
                    IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::removeClient",  "client count: %u", getClientCount());
                    break;
                }
            }
            mMutex.unlock();
        }


        void CNetServerSeniorTCP::removeAllClient() {
            mMutex.lock();
            for(u32 i=0; i<mAllContextSocket.size(); ++i ) {
                delete mAllContextSocket[i];
            }
            mAllContextSocket.clear();
            mMutex.unlock();
        }


        bool CNetServerSeniorTCP::clearError(SClientContext* pContext) {
            s32 ecode = errno;
            switch(ecode) {
            case EAGAIN:
                //case EWOULDBLOCK:
                break;

            case ECONNRESET:
                removeClient(pContext);
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::clearError",  "error ECONNRESET: %d", ecode);
                return false;
            default:
                removeClient(pContext);
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::clearError",  "error other: %d", ecode);
                return false;
            }//switch
            return true;
        }


        bool CNetServerSeniorTCP::stepReceive(SClientContext* iContextSocket, SContextIO* iContextIO) {
            //
            s32 ret = recv(iContextIO->mContext->mSocket, iContextIO->mNetPack.getWritePointer(), iContextIO->mNetPack.getWriteSize(), 0);
            sockaddr_in* iClientAddr = &iContextSocket->mClientAddress;
            //net::CNetPacket packSend;
            net::CNetPacket& pack = iContextIO->mNetPack;
            u8 echoU8 = ENET_NULL;
            if(ret>0) {
                if(pack.init(ret)) {
                    APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::stepReceive", "net package init success: [%u]", iContextIO->mBytes);
                    do {
                        switch(pack.readU8()) {
                        case ENET_DATA: {
                            APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::stepReceive",  "received=[%s:%d] info=[data]",
                                inet_ntoa(iClientAddr->sin_addr), ntohs(iClientAddr->sin_port));
                            if(mReceiver) {
                                mReceiver->onReceive(pack);
                            }
                            echoU8 = (u8)ENET_DATA;
                            break;
                                        }
                        case ENET_WAIT: {
                            APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::stepReceive",  "received=[%s:%d] info=[wait]",
                                inet_ntoa(iClientAddr->sin_addr), ntohs(iClientAddr->sin_port));
                            echoU8 = (u8)ENET_DATA;
                            break;
                                        }
                        case ENET_HI: {
                            APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::stepReceive",  "received=[%s:%d] info=[hi]",
                                inet_ntoa(iClientAddr->sin_addr), ntohs(iClientAddr->sin_port));
                            echoU8 = (u8)ENET_DATA;
                            break;
                                      }
                        default: {
                            APP_LOG(ELOG_DEBUG, "CNetServerSeniorTCP::stepReceive",  "received=[%s:%d] info=[unknown]",
                                inet_ntoa(iClientAddr->sin_addr), ntohs(iClientAddr->sin_port));
                            return postReceive(iContextIO);
                                 }
                        }//switch
                    } while(pack.slideNode());
                } else {
                    IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stepReceive", "net package init failed: [%ld][%u]", ret, pack.getSize());
                    return postReceive(iContextIO);
                }
            } else if(0==ret) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::stepReceive", "client quit");
                removeClient(iContextSocket);
                return false;
            } else if(!clearError(iContextSocket)){
                return false;
            }

            pack.clear();
            pack.add(echoU8);
            pack.finish();
            return postSend(iContextIO);
        }


        bool CNetServerSeniorTCP::stepSend(SClientContext* iContextSocket, SContextIO* iContextIO) {
            s32 iGotSize = send(iContextIO->mContext->mSocket,iContextIO->mNetPack.getPointer(),iContextIO->mNetPack.getSize(),0);
            if(APP_SOCKET_ERROR == iGotSize) {
                if(!clearError(iContextSocket)){
                    return false;
                }
            } else {
                IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stepSend", "send seccuss");
            }
            if(!postReceive(iContextIO)) {
                iContextIO->mContext->removeContextIO(iContextIO);
                return false;
            }
            return true;
        }


        bool CNetServerSeniorTCP::stepAccpet(SClientContext* iContextSocket, SContextIO* iContextIO) {
            //TODO>>accpet until error
            SClientContext* newContextSocket = new SClientContext();
            u32 len=sizeof(newContextSocket->mClientAddress);
            newContextSocket->mSocket = accept(iContextIO->mContext->mSocket,(sockaddr*)&newContextSocket->mClientAddress,&len);
            if(APP_INVALID_SOCKET == newContextSocket->mSocket) {
                IAppLogger::log(ELOG_ERROR, "CNetServerSeniorTCP::stepAccpet",  "error: %d", errno);
                delete newContextSocket;
                return false;
            }

            SContextIO* newIO = newContextSocket->getNewContexIO();
            registeEvent(newIO, EPOLL_CTL_ADD, EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP);
            if(!postReceive(newIO)) {
                delete newContextSocket;
                return false;
            }

            IAppLogger::log(ELOG_INFO, "CNetServerSeniorTCP::stepAccpet",  "incoming client=[%s:%d]",
                inet_ntoa(newContextSocket->mClientAddress.sin_addr),
                ntohs(newContextSocket->mClientAddress.sin_port));

            addClient(newContextSocket);
            return true;
        }


        bool CNetServerSeniorTCP::postAccept(SContextIO* iContextIO) {
            iContextIO->reset(EOP_ACCPET);
            registeEvent(iContextIO, EPOLL_CTL_ADD, EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP);
            return true;
        }


        bool CNetServerSeniorTCP::postReceive(SContextIO* iContextIO) {
            iContextIO->reset(EOP_RECEIVE);
            registeEvent(iContextIO, EPOLL_CTL_MOD, EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP);
            return true;
        }


        bool CNetServerSeniorTCP::postSend(SContextIO* iContextIO) {
            iContextIO->reset(EOP_SEND);
            registeEvent(iContextIO, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP);
            return true;
        }



    }// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_LINUX
