#ifndef APP_CNETCLIENTTCP_H
#define APP_CNETCLIENTTCP_H

#include "INetClient.h"
#include "irrString.h"
#include "CStreamFile.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <Windows.h>
#elif defined(APP_PLATFORM_LINUX)

#endif



namespace irr {

    namespace net {

        class CNetClientTCP : public INetClient {
        public:
            CNetClientTCP();

            virtual ~CNetClientTCP();

            virtual ENetNodeType getType() const {
                return ENET_TCP_CLIENT;
            }

            virtual void setNetEventer(INetEventer* it){
                mReceiver = it;
            }

            virtual bool update(u32 iTime);

            virtual bool start();

            virtual bool stop();

            virtual void setIP(const c8* ip);

            virtual void setPort(u16 port);

            virtual s32 sendData(const c8* iData, s32 iLength);

            virtual s32 sendData(const net::CNetPacket& iData);

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

        private:
            void resetSocket();
            void linkSocket();
            void sendTick();

            /**
            *@return True if not really error, else false.
            */
            bool clearError();

            /**
            *@return True if a package had been sent completely, else false.
            *@note Can't write other data into socket cache if return false.
            */
            bool flushDatalist();

            void setNetPackage(ENetMessage it, net::CNetPacket& out);


            bool mRunning;
            u8 mStatus;
            u16 mPort;
            u16 mMyPort;
            u32 mTickTime;                   ///<Heartbeat time
            u32 mTickCount;                 ///<Heartbeat count
            u32 mLastUpdateTime;    ///<last absolute time of update
            netsocket mSocket;
            INetEventer* mReceiver;
            core::stringc mIP;
            core::stringc mMyIP;
            io::CStreamFile mStream;
            sockaddr_in mRemoteAddress;
        };

    }// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTTCP_H
