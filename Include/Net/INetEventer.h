#ifndef APP_INETEVENTER_H
#define APP_INETEVENTER_H

namespace irr {
    namespace net {

        class CNetPacket;

        class INetEventer {
        public:
            INetEventer(){
            }
            virtual ~INetEventer(){
            }
            /**
            *@brief Callback function when received net packet.
            *@param The received package.
            */
            virtual void onReceive(CNetPacket& it) = 0;

            //virtual void onSend(net::CNetPacket& it) = 0;
        };

    }// end namespace net
}// end namespace irr

#endif //APP_INETEVENTER_H
