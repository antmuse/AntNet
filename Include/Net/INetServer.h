#ifndef APP_INETSERVER_H
#define APP_INETSERVER_H

#include "HNetConfig.h"
#include "INetEventer.h"
#include "IPairOrderID.h"

namespace irr {
    namespace net {

        ///a client id pair(IP, Port)
        typedef IPairOrderID<u32, u32> CNetClientID;


        ///net operation type
        enum EOperationType {
            EOP_NULL,                        ///<Used to init or reset
            EOP_ACCPET=1,                    ///<Accept operation
            EOP_SEND = 2,                       ///<Send operation
            EOP_RECEIVE = 4,                   ///<Receive operation
        };


        class INetServer {
        public:
            INetServer(){
            }
            virtual ~INetServer(){
            }

            virtual ENetNodeType getType() const =0;

            virtual void setNetEventer(INetEventer* it) = 0;

            virtual bool start() = 0;

            virtual bool stop() = 0;

            virtual void setIP(const c8* ip) = 0;

            virtual void setPort(u16 port) = 0;

            virtual const c8* getIP() const = 0;

            virtual u16 getPort() const = 0;

            virtual const c8* getLocalIP() const = 0;

            virtual u16 getLocalPort() const = 0;

            virtual void setMaxClients(u32 max) = 0;

            virtual void setThreads(u32 max) = 0;

            virtual u32 getClientCount() const = 0;

            //virtual s32 sendData(const net::CNetPacket& iData) = 0;

            //virtual s32 sendData(const c8* iData, s32 iLength) = 0;
        };

    }// end namespace net
}// end namespace irr

#endif //APP_INETSERVER_H
