#include "CNetUtility.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif


namespace app {
namespace net {

void CNetUtility::setLastError(s32 it) {
#if defined(APP_PLATFORM_WINDOWS)
    ::WSASetLastError(it);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    errno = it;
#endif
}


s32 CNetUtility::getSocketError() {
#if defined(APP_PLATFORM_WINDOWS)
    return ::WSAGetLastError();  // GetLastError();
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return errno;
#endif
}


s32 CNetUtility::getLastError() {
#if defined(APP_PLATFORM_WINDOWS)
    return ::GetLastError();
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return errno;
#endif
}


s32 CNetUtility::loadSocketLib() {
#if defined(APP_PLATFORM_WINDOWS)
    WSADATA wsaData;
    return ::WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif //APP_PLATFORM_WINDOWS

    return 0;
}


s32 CNetUtility::unloadSocketLib() {
#if defined(APP_PLATFORM_WINDOWS)
    return ::WSACleanup();
#endif //APP_PLATFORM_WINDOWS

    return 0;
}

} //namespace net
} //namespace app
