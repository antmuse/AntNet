#ifndef APP_CNETUTILITY_H
#define APP_CNETUTILITY_H

#include "HNetConfig.h"

namespace irr{
    namespace net{

        class CNetUtility{
        public:

            static bool setReuseIP(netsocket iSocket);

            static bool setReusePort(netsocket iSocket);

            static bool setNoblock(netsocket iSocket);

            /**
            *@brief Close a valid socket.
            *@param iSocket The socket.
            *@return 0 if success, else if failed.
            */
            static s32 closeSocket(netsocket iSocket);

            /**
            *@brief Set send cache size of socket.
            *@param iSocket The socket.
            *@return 0 if success, else if failed.
            */
            static s32 setSendCache(netsocket iSocket, s32 size);

            /**
            *@brief Set receive cache size of socket.
            *@param iSocket The socket.
            *@return 0 if success, else if failed.
            */
            static s32 setReceiveCache(netsocket iSocket, s32 size);


#if defined(APP_PLATFORM_WINDOWS)
            static s32 loadSocketLib();
            static s32 unloadSocketLib();
#endif  //APP_PLATFORM_WINDOWS


        private:
            CNetUtility(){
            };

            ~CNetUtility(){
            };
        };


    }// end namespace net
}// end namespace irr


#endif //APP_CNETUTILITY_H