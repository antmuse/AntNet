#ifndef APP_CNETSERVERNORMALTCP_H
#define APP_CNETSERVERNORMALTCP_H

#include "INetServer.h"
#include "irrString.h"
#include "irrArray.h"
#include "IRunnable.h"


namespace irr {
    class CMutex;
    class CThread;

    namespace net {
        class CNetServerNormalTCP;

        class CNetServerThread : public IRunnable {
        public:
            CNetServerThread(CNetServerNormalTCP& iHub, netsocket iNode);

            virtual ~CNetServerThread();

            void setSocket(netsocket iNode);

            virtual void run();

            bool start();

            bool stop();

        private:
            u8 mStatus;
            bool mRunning;
            netsocket mSocket;
            CThread* mThread;
            CNetServerNormalTCP& mHub;
        };


        /**
        * @brief This server is for testing currently, should be improved in future.
        */
        class CNetServerNormalTCP : public INetServer, public IRunnable {
        public:
            CNetServerNormalTCP();

            virtual ~CNetServerNormalTCP();

            virtual ENetNodeType getType() const {
                return ENET_TCP_SERVER_NORMALL;
            }

            virtual void setNetEventer(INetEventer* it){
                mReceiver = it;
            }

            INetEventer* getNetEventer() const {
                return mReceiver;
            }

            virtual void run();

            virtual bool start();

            virtual bool stop();

            virtual void setIP(const c8* ip);

            virtual void setPort(u16 port);

            virtual const c8* getIP() const {
                return mIP.c_str();
            }

            virtual u16 getPort() const {
                return mPort;
            }

            virtual const c8* getLocalIP() const {
                return mMyIP.c_str();
            }

            virtual u16 getLocalPort() const{
                return mMyPort;
            }

            virtual void setMaxClients(u32 max){
                mMaxClients = max;
            }

            virtual u32 getClientCount() const {
                return mClientCount;
            }

            void removeWorker(CNetServerThread* it);

            virtual void setThreads(u32 max){
                mThreadCount = max;
            }

        private:
            bool init();

            core::stringc mMyIP;
            core::stringc mIP;
            bool mRunning;
            u16 mPort;
            u16 mMyPort;
            u32 mMaxClients;
            u32 mClientCount;
            u32 mThreadCount;
            netsocket mSocketServer;
            core::array<CNetServerThread*> mAllClient;
            core::array<CNetServerThread*> mAllClientWait;
            CThread* mThread;
            CMutex* mMutex;
            INetEventer* mReceiver;
        };

    }// end namespace net
}// end namespace irr

#endif //APP_CNETSERVERNORMALTCP_H
