#include "CNetUtility.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#elif defined(APP_PLATFORM_LINUX)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif


namespace irr{
    namespace net{


#if defined(APP_PLATFORM_WINDOWS)

        s32 CNetUtility::loadSocketLib(){
            WSADATA wsaData;
            return WSAStartup(MAKEWORD(2, 2), &wsaData);
        }


        s32 CNetUtility::unloadSocketLib(){
            return WSACleanup();
        }


        s32 CNetUtility::closeSocket(netsocket iSocket){
            return closesocket(iSocket);
        }


        bool CNetUtility::setNoblock(netsocket iSocket){
            u_long ul = 1;
            return 0 == ioctlsocket(iSocket, FIONBIO, &ul);
        }


        bool CNetUtility::setReuseIP(netsocket iSocket){
            s32 opt = 1;
            if(setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (c8*)&opt, sizeof(opt)) < 0){
                return false;
            }
            return true;
        }


        bool CNetUtility::setReusePort(netsocket iSocket){
            //windows have no SO_REUSEPORT
            //s32 opt = 1;
            //if(setsockopt(iSocket, SOL_SOCKET, SO_REUSEPORT, (c8*)&opt, sizeof(opt)) < 0){
            //    return false;
            //}
            return true;
        }


        s32 CNetUtility::setReceiveCache(netsocket iSocket, s32 size){
            return setsockopt(iSocket, SOL_SOCKET, SO_RCVBUF, (c8*)&size, sizeof(size));
        }


        s32 CNetUtility::setSendCache(netsocket iSocket, s32 size){
            return setsockopt(iSocket, SOL_SOCKET, SO_SNDBUF, (c8*)&size, sizeof(size));
        }


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)


        s32 CNetUtility::closeSocket(netsocket iSocket){
            return close(iSocket);
        }

        bool CNetUtility::setNoblock(netsocket sock){
            s32 opts = fcntl(sock, F_GETFL);
            if(opts < 0){
                //IAppLogger::log(ELOG_ERROR, "CNetUtility::setNoblock", "F_GETFL fail");
                return false;
            }
            opts = opts | O_NONBLOCK;
            if(fcntl(sock, F_SETFL, opts) < 0){
                //IAppLogger::log(ELOG_ERROR, "CNetUtility::setNoblock", "F_SETFL fail");
                return false;
            }
            return true;
        }

        bool CNetUtility::setReuseIP(netsocket iSocket){
            s32 opt = 1;
            if(setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (c8*)&opt, sizeof(opt)) < 0){
                return false;
            }
            return true;
        }


        bool CNetUtility::setReusePort(netsocket iSocket){
            s32 opt = 1;
            if(setsockopt(iSocket, SOL_SOCKET, SO_REUSEPORT, (c8*)&opt, sizeof(opt)) < 0){
                return false;
            }
            return true;
        }
#endif


    }// end namespace net
}// end namespace irr