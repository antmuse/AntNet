#ifndef APP_INETMANAGER_H
#define APP_INETMANAGER_H

#include "INetClient.h"
#include "INetServer.h"

namespace irr {
    namespace net {

        class INetManager {
        public:
            virtual INetServer* createServerNormalTCP() = 0;

            virtual INetServer* createServerNormalUDP() = 0;

            virtual INetServer* createServerSeniorTCP() = 0;

            virtual INetServer* createServerSeniorUDP() = 0;

            virtual INetClient* addClientTCP() = 0;

            virtual INetClient* addClientUDP() = 0;

            virtual INetClient* getClientTCP(u32 index) = 0;

            virtual INetClient* getClientUDP(u32 index) = 0;

            virtual bool start() = 0;

            virtual bool stop() = 0;

        protected:
            INetManager(){
            }

            virtual ~INetManager(){
            }
        };



        /**
        *@brief Get the unique net manager.
        *@return Net manager
        */
        INetManager* AppGetNetManagerInstance();


    }// end namespace net
}// end namespace irr

#endif //APP_INETMANAGER_H
