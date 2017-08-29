/**
*@file CNetServerSeniorUDP.h
*Defined CNetServerSeniorUDP, SContextIO, SClientContextUDP
*/

#ifndef APP_CNETSERVERSENIORUDP_H
#define APP_CNETSERVERSENIORUDP_H


#include "INetServer.h"
#include "irrString.h"
#include "irrMap.h"
#include "IRunnable.h"
#include "SClientContext.h"
#include "CThread.h"

#if defined(APP_PLATFORM_WINDOWS)

namespace irr {
    namespace net {
        /**
        *@class CNetServerSeniorUDP
        */
        class CNetServerSeniorUDP : public INetServer, public IRunnable , public INetDataSender {
        public:
            CNetServerSeniorUDP();

            virtual ~CNetServerSeniorUDP();

            virtual ENetNodeType getType() const {
                return ENET_UDP_SERVER_SENIOR;
            }

            virtual void setNetEventer(INetEventer* it){
                mReceiver = it;
            }

            virtual s32 sendBuffer(void* iUserPointer, const s8* iData, s32 iLength);

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

            /**
            *@brief Request a receive action.
            *@param iContext,
            *@return True if request finished, else false.
            */
            bool stepReceive(SClientContextUDP* iContext);


            void addClient(SClientContextUDP* iContext);


            void removeClient(CNetClientID id);


            bool clearError();

            virtual void setThreads(u32 max) {
            }

        protected:
            bool initialize();
            void removeAll();
            void removeAllClient();

        private:

            core::stringc mMyIP;                               ///<My IP
            core::stringc mIP;                                      ///<Server's IP
            u16 mMyPort;
            u16 mPort;
            bool mRunning;                                       ///<True if server started, else false
            u32 mMaxClientCount;
            u32 mTime;
            CMutex mMutex;
            core::map<CNetClientID, SClientContextUDP*>  mAllContextSocket;          ///<All the clients's socket context    TODO-use core::map
            CThread* mThread;								                        ///<busy worker
            netsocket mSocket;											        ///<listen socket
            INetEventer* mReceiver;
            //CMemoryHub mMemHub;
        };


    }// end namespace net
}// end namespace irr
#endif //APP_PLATFORM_WINDOWS





#if defined(APP_PLATFORM_LINUX)

#endif //APP_PLATFORM_LINUX

#endif //APP_CNETSERVERSENIORUDP_H
