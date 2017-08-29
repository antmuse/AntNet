#ifndef APP_CNETCLIENTUDP_H
#define APP_CNETCLIENTUDP_H

#include "INetClient.h"
#include "IRunnable.h"
#include "irrString.h"
#include "CNetProtocal.h"
#include "CNetPacket.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <Windows.h>
#elif defined(APP_PLATFORM_LINUX)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace irr {
	class CThread;

    namespace net {
        class CNetClientUDP : public INetClient, public INetDataSender {
        public:

            CNetClientUDP();

            virtual ~CNetClientUDP();


            virtual ENetNodeType getType() const {
				return ENET_UDP_CLIENT;
			}


            virtual void setNetEventer(INetEventer* it){
                mReceiver = it;
            }

            /**
            *@brief Low level send function for CNetProtocal;
            *@override INetDataSender::sendBuffer();
            *@see INetDataSender
            */
            virtual s32 sendBuffer(void* iUserPointer, const s8* iData, s32 iLength);

            bool update(u32 iTime);

            virtual bool start();

            virtual bool stop();

            virtual void setIP(const c8* ip);

            virtual void setPort(u16 port);

            virtual s32 sendData(const c8* iData, s32 iLength);

            virtual s32 sendData(const net::CNetPacket& iData);

            virtual const c8* getIP() const {
                return mRemoteIP.c_str();
            }

            virtual u16 getPort() const {
                return mRemotePort;
            }
            
            virtual const c8* getLocalIP() const {
                return mMyIP.c_str();
            }

            virtual u16 getLocalPort() const{
                return mMyPort;
            }

            void setID(u32 id);


            u32 getID() const {
                return mProtocal.getID();
            }


        private:
            /**
            *@return True if not really error, else false.
            */
            bool clearError();

            bool bindLocal();

            void setNetPackage(ENetMessage it, net::CNetPacket& out);

            void resetSocket();

            void sendTick();

            void sendBye();

            /**
            *@param iStepTime relative time step
            */
            void step(u32 iStepTime);


            bool mRunning;
            u8 mStatus;
			u16 mMyPort;
            u16 mRemotePort;
            u32 mUpdateTime;
            u32 mTickTime;
            u32 mTickCount;
            netsocket mSocket;
            sockaddr_in mRemoteAddress;
            INetEventer* mReceiver;
            CNetProtocal mProtocal;
            CNetPacket mPacket;
			core::stringc mMyIP;
            core::stringc mRemoteIP;
        };

    }// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTUDP_H
