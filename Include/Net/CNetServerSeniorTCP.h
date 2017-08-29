/**
*@file CNetServerSeniorTCP.h
*Defined CNetServerSeniorTCP, SContextIO, SClientContext
*/

#ifndef APP_CNETSERVERSENIORTCP_H
#define APP_CNETSERVERSENIORTCP_H


#include "INetServer.h"
#include "irrString.h"
#include "IRunnable.h"
#include "SClientContext.h"
#include "CThread.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <MSWSock.h>

namespace irr {
	namespace net {

		/**
		*@class CNetServerSeniorTCP
		*/
		class CNetServerSeniorTCP : public INetServer, public IRunnable {
		public:
			CNetServerSeniorTCP();

			virtual ~CNetServerSeniorTCP();

			virtual ENetNodeType getType() const {
				return ENET_TCP_SERVER_SENIOR;
			}
			
			virtual void setNetEventer(INetEventer* it){
				mReceiver = it;
			}
			
            virtual void run();

			virtual bool start();

			virtual bool stop();

			virtual void setIP(const c8* ip);

			virtual const c8* getIP() const {
				return mIP.c_str();
			};

			virtual u16 getPort() const {
				return mPort;
			};

            virtual const c8* getLocalIP() const {
                return mMyIP.c_str();
            }

            virtual u16 getLocalPort() const{
                return mMyPort;
            }

			virtual void setMaxClients(u32 max) {
				mMaxClientCount = max;
			};

			virtual u32 getClientCount() const {
				return mAllContextSocket.size();
			}

			virtual void setPort(u16 iPort ) { 
				mPort=iPort; 
			}

			APP_FORCE_INLINE HANDLE getHandleIOCP() const {
				return mHandleIOCP;
			}

			/**
			*@brief Request an accept action.
			*@param iContextIO,
			*@return True if request finished, else false.
			*/
			bool postAccept(SClientContext* iContext, SContextIO* iAction);

			/**
			*@brief Request a receive action.
			*@param iContextIO,
			*@return True if request finished, else false.
			*/
			bool postReceive(SClientContext* iContext, SContextIO* iAction);

			/**
			*@brief Request a send action.
			*@param iContext,
			*@param iAction,
			*@param userPost True is means it's not posted in ::stepSend();
			*@return True if request finished, else false.
			*/
			bool postSend(SClientContext* iContext, SContextIO* iAction, bool userPost = true);

			/**
			*@brief Go to next step when got the IO finished info: accpet.
			*@param iContextSocket,
			*@param iContextIO,
			*@return True if next step finished, else false.
			*/
			bool stepAccpet(SClientContext* iContext, SContextIO* iAction);

			/**
			*@brief Go to next step when got the IO finished info: receive.
			*@param iContextSocket,
			*@param iContextIO,
			*@return True if next step finished, else false.
			*/
			bool stepReceive(SClientContext* iContext, SContextIO* iAction);

			/**
			*@brief Go to next step when got the IO finished info: send.
			*@param iContextSocket,
			*@param iContextIO,
			*@return True if next step finished, else false.
			*/
			bool stepSend(SClientContext* iContext, SContextIO* iAction);


			void addClient(SClientContext* iContext);


			void removeClient(SClientContext* iContext);


			bool clearError(SClientContext* iContext);

			/**
			*@brief The work thread will call this by itself when it's finished.
			*@param it, The work thread which are quitting now.
			*/
			void onThreadQuit();


            virtual void setThreads(u32 max){
                mThreadCount = max;
            }

		protected:
			bool initialize();
			bool initializeSocket();
			void removeAll();
			void removeAllClient();
			bool bindWithIOCP(SClientContext* iContext);
			bool isSocketAlive(netsocket s);
			bool addToSendList(SClientContext* iContext, CNetPacket& pack);

		private:
			core::stringc mMyIP;                               ///<My IP
			core::stringc mIP;                                  ///<Server's IP
			u16 mMyPort;
			u16 mPort;
			bool mRunning;                                  ///<True if server started, else false
			u32 mThreadCount;
			u32 mMaxClientCount;
			void* mHandleIOCP;
			CMutex mMutex;
			core::array<SClientContext*>  mAllContextSocket;          ///<All the clients's socket context    TODO-use core::map
			core::array<CThread*>  mAllWorker;								///<All workers
			SClientContext* mListenContext;											///<listen socket's context
			INetEventer* mReceiver;
			//CMemoryHub mMemHub;
			LPFN_ACCEPTEX mLPFNAcceptEx;											///<AcceptEx function pointer
			LPFN_GETACCEPTEXSOCKADDRS    mLPFNGetAcceptExSockAddress; ///<GetAcceptExSockaddrs  function pointer
		};


	}// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_WINDOWS





#if defined(APP_PLATFORM_LINUX)
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "CThread.h"


namespace irr {
	namespace net {

		///IOCP operation type
		enum EOperationType {
			EOP_NULL,                        ///<Used to init or reset
			EOP_ACCPET,                    ///<Accept operation
			EOP_SEND,                       ///<Send operation
			EOP_RECEIVE                   ///<Receive operation
		};

		struct SClientContext;

		///Socket IO contex
		struct SContextIO {
			EOperationType mOperationType;      				///<Current operation type.
			SClientContext* mContext;
			CNetPacket mNetPack;                         		///<The real cache.

			SContextIO(SClientContext* iContext) : mContext(iContext),
				mOperationType(EOP_NULL){
					mNetPack.reallocate(APP_NET_MAX_BUFFER_LEN);
			}

			~SContextIO() {

			}

			void reset(EOperationType it) {
				mOperationType = it;
				switch(it){
				case EOP_SEND:
					//mBufferWSA.buf = mNetPack.getPointer();
					//mBufferWSA.len = mNetPack.getSize();
					break;
				case EOP_RECEIVE:
					//mBufferWSA.buf = mNetPack.getWritePointer();
					//mBufferWSA.len = mNetPack.getWriteSize();
					break;
				case EOP_ACCPET:
					mNetPack.clear();
					//mBufferWSA.buf = mNetPack.getPointer();
					//mBufferWSA.len = APP_NET_MAX_BUFFER_LEN;
					break;
				default:
					mOperationType = EOP_NULL;
					mNetPack.clear();
					//mBufferWSA.buf = mNetPack.getPointer();
					//mBufferWSA.len = APP_NET_MAX_BUFFER_LEN;
					break;
				}//switch
			}
		} ;


		///Socket contex of each client
		struct SClientContext {
			s32      mSocket;												///<Socket of client
			sockaddr_in mClientAddress;                           	///<Client address
			core::array<SContextIO*> mAllIOContext;    ///<All IO context of client, you can post more than 1 IO request on each client.

			SClientContext() : mSocket(APP_INVALID_SOCKET) {
				memset(&mClientAddress, 0, sizeof(mClientAddress));
			}

			~SClientContext(){
				if(APP_INVALID_SOCKET != mSocket){
					close( mSocket );
					mSocket = APP_INVALID_SOCKET;
				}
				for(u32 i=0; i<mAllIOContext.size(); ++i){
					delete mAllIOContext[i];
				}
				//mAllIOContext.clear();
				mAllIOContext.set_used(0);
			}

			SContextIO* getNewContexIO(){
				SContextIO* p = new SContextIO(this);
				mAllIOContext.push_back(p);
				return p;
			}

			void removeContextIO(SContextIO* iContext ) {
				//ASSERT( iContext!=0 );
				for(u32 i=0; i<mAllIOContext.size(); ++i){
					if(iContext==mAllIOContext[i]){
						//mAllIOContext.erase(i);
						mAllIOContext[i] = mAllIOContext.getLast();
						mAllIOContext.set_used(mAllIOContext.size()-1);
						delete iContext;
						break;
					}
				}//for
			}

		};



		class CNetServerSeniorTCP;
		class CNetWorker : public IRunnable{
		public:
			CNetWorker(CNetServerSeniorTCP& iServer);
			virtual ~CNetWorker();

			virtual void run();

			void start();
			void stop();

		private:
			bool mRunning;
			u32 mID;
			CNetServerSeniorTCP& mServer;
			CThread mThread;
		};


		/**
		*@class CNetServerSeniorTCP
		*/
		class CNetServerSeniorTCP : public INetServer {
		public:
			CNetServerSeniorTCP();
			virtual ~CNetServerSeniorTCP();

			virtual ENetNodeType getType() const {
				return ENET_SERVER_SENIOR;
			}
			virtual void setNetEventer(INetEventer* it){
				mReceiver = it;
			}
			virtual bool start();
			virtual bool stop();

			virtual bool sendData(const net::CNetPacket& iData);

			virtual bool sendData(const c8* iData, s32 iLength);

			virtual void setIP(const c8* ip);

			virtual const c8* getIP() const {
				return mIP.c_str();
			};

			virtual u16 getPort() const {
				return mPort;
			};

			virtual void setMaxClients(u32 max) {
				mMaxClientCount = max;
			};

			virtual u32 getClientCount() const {
				return mAllContextSocket.size();
			}

			virtual void setPort(u16 iPort ) {
				mPort=iPort;
			}

			void removeWorker(CNetWorker* it);

			APP_FORCE_INLINE s32 getHandle() const {
				return mEpollFD;
			}

			void addClient(SClientContext* iContextSocket );

			void removeClient(SClientContext* iContextSocket);

			void removeAllClient();

			bool clearError(SClientContext* pContext);

			/**
			*@brief Request an accept action.
			*@param iContextIO,
			*@return True if request finished, else false.
			*/
			bool postAccept(SContextIO* iContextIO);

			/**
			*@brief Request a receive action.
			*@param iContextIO,
			*@return True if request finished, else false.
			*/
			bool postReceive(SContextIO* iContextIO);

			/**
			*@brief Request a send action.
			*@param iContextIO,
			*@return True if request finished, else false.
			*/
			bool postSend(SContextIO* iContextIO);

			/**
			*@brief Go to next step when got the IO finished info: accpet.
			*@param iContextSocket,
			*@param iContextIO,
			*@return True if next step finished, else false.
			*/
			bool stepAccpet(SClientContext* iContextSocket, SContextIO* iContextIO);

			/**
			*@brief Go to next step when got the IO finished info: receive.
			*@param iContextSocket,
			*@param iContextIO,
			*@return True if next step finished, else false.
			*/
			bool stepReceive(SClientContext* iContextSocket, SContextIO* iContextIO);

			/**
			*@brief Go to next step when got the IO finished info: send.
			*@param iContextSocket,
			*@param iContextIO,
			*@return True if next step finished, else false.
			*/
			bool stepSend(SClientContext* iContextSocket, SContextIO* iContextIO);


		private:
			bool initialize();
			void removeAll();
			//action = EPOLL_CTL_ADD, EPOLL_CTL_DEL, EPOLL_CTL_MOD
			bool registeEvent(SContextIO* iContextIO, s32 action, u32 mask);

			core::stringc mIP;                              ///<Server's IP
			u16 mPort;
			bool mRunning;                                  ///<True if server started, else false
			u32 mMaxWorkerCount;
			u32 mMaxClientCount;
			s32 mEpollFD;
			CMutex mMutex;
			core::array<SClientContext*>  mAllContextSocket;          ///<All the clients's socket context
			core::array<CNetWorker*>  mAllWorker;                     ///<All workers
			SClientContext* mListenContext;                     ///<listen socket's context
			INetEventer* mReceiver;
		};

	}// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_LINUX

#endif //APP_CNETSERVERSENIORTCP_H
