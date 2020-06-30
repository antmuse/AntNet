#include "CNetAddress.h"
#include "fast_atof.h"
//#include "IUtility.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <WS2tcpip.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif  //APP_PLATFORM_WINDOWS


namespace app {
namespace net {

APP_INLINE void CNetAddress::init() {
    ::memset(mAddress, 0, sizeof(mAddress));
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(28 == sizeof(sockaddr_in6));
    getAddress()->sin6_family = AF_INET6;
#else
    APP_ASSERT(16 == sizeof(sockaddr_in));
    getAddress()->sin_family = AF_INET;
#endif
    //printf("sizeof(sockaddr_in)=%u\n", sizeof(sockaddr_in));
    //printf("sizeof(sockaddr_in6)=%u\n", sizeof(sockaddr_in6));
}


CNetAddress::CNetAddress() :
    mID(0),
    mPort(APP_NET_ANY_PORT) {
    ::strcpy(mIP, APP_NET_ANY_IP);
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const s8* ip) :
    mID(0),
    mPort(APP_NET_ANY_PORT) {
    if(ip) {
        ::memcpy(mIP, ip, core::min_<size_t>(APP_IP_STRING_MAX_SIZE - sizeof(s8), ::strlen(ip) + sizeof(s8)));
        mIP[APP_IP_STRING_MAX_SIZE - sizeof(s8)] = '\0';
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const core::CString& ip) :
    mID(0),
    mPort(APP_NET_ANY_PORT) {
    if(ip.size() < APP_IP_STRING_MAX_SIZE) {
        ::memcpy(mIP, ip.c_str(), ip.size() + sizeof(s8));
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(u16 port) :
    mID(0),
    mPort(port) {
    ::strcpy(mIP, APP_NET_ANY_IP);
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const s8* ip, u16 port) :
    mID(0),
    mPort(port) {
    if(ip) {
        ::memcpy(mIP, ip, core::min_<size_t>(APP_IP_STRING_MAX_SIZE - sizeof(s8), ::strlen(ip) + sizeof(s8)));
        mIP[APP_IP_STRING_MAX_SIZE - sizeof(s8)] = '\0';
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }

    init();
    initIP();
    initPort();
}


CNetAddress::~CNetAddress() {
}


CNetAddress::CNetAddress(const core::CString& ip, u16 port) :
    mID(0),
    mPort(port) {
    if(ip.size() < APP_IP_STRING_MAX_SIZE) {
        ::memcpy(mIP, ip.c_str(), ip.size() + sizeof(s8));
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const CNetAddress& other) {
    ::memcpy(mAddress, other.mAddress, sizeof(mAddress));
    ::memcpy(mIP, other.mIP, sizeof(mIP));
    mPort = other.mPort;
    mID = other.mID;
}


CNetAddress& CNetAddress::operator=(const CNetAddress& other) {
    if(this == &other) {
        return *this;
    }
    ::memcpy(mAddress, other.mAddress, sizeof(mAddress));
    ::memcpy(mIP, other.mIP, sizeof(mIP));
    mID = other.mID;
    mPort = other.mPort;
    return *this;
}


bool CNetAddress::operator==(const CNetAddress& other) const {
    return (mID == other.mID);
}


bool CNetAddress::operator!=(const CNetAddress& other) const {
    return (mID != other.mID);
}


void CNetAddress::setIP(const s8* ip) {
    if(ip) {
        ::memcpy(mIP, ip, core::min_<size_t>(APP_IP_STRING_MAX_SIZE - sizeof(s8), ::strlen(ip) + sizeof(s8)));
        mIP[APP_IP_STRING_MAX_SIZE - sizeof(s8)] = '\0';
        initIP();
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
}


bool CNetAddress::setIP() {
    s8 host_name[256];
    if(::gethostname(host_name, sizeof(host_name) - 1) < 0) {
        return false;
    }
    struct addrinfo* head = 0;
    struct addrinfo hints;
#if defined(APP_NET_USE_IPV6)
    hints.ai_family = getAddress()->sin6_family;
#else
    hints.ai_family = getAddress()->sin_family;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;    // IPPROTO_TCP; //0，默认
    hints.ai_flags = AI_PASSIVE;        //flags的标志很多,常用的有AI_CANONNAME;
    hints.ai_addrlen = 0;
    hints.ai_canonname = 0;
    hints.ai_addr = 0;
    hints.ai_next = 0;
    s32 ret = ::getaddrinfo(host_name, 0, &hints, &head);
    if(ret) {
        return false;
    }
    //while(curr && curr->ai_canonname)
    for(struct addrinfo* curr = head; curr; curr = curr->ai_next) {
#if defined(APP_NET_USE_IPV6)
        if(curr->ai_family == getAddress()->sin6_family) {
            struct sockaddr_in6* sockaddr = (struct sockaddr_in6*) curr->ai_addr;
            getAddress()->sin6_addr = sockaddr->sin6_addr;
            ::inet_ntop(getAddress()->sin6_family, &getAddress()->sin6_addr, mIP, sizeof(mIP));
            mergeIP();
            //APP_LOG(ELOG_INFO, "CNetAddress::setIP", "IPV6 = %s", mIP.c_str());
            break;
        }
#else
        if(curr->ai_family == getAddress()->sin_family) {
            struct sockaddr_in* sockaddr = (struct sockaddr_in*) curr->ai_addr;
            getAddress()->sin_addr = sockaddr->sin_addr;
            ::inet_ntop(getAddress()->sin_family, &getAddress()->sin_addr, mIP, sizeof(mIP));
            mergeIP();
            //APP_LOG(ELOG_INFO, "CNetAddress::setIP", "IPV4 = %s", mIP.c_str());
            break;
        }
#endif
    } //for

    ::freeaddrinfo(head);
    return true;
}


void CNetAddress::setIP(const IP& ip) {
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(sizeof(ip) == sizeof(getAddress()->sin6_addr));
    *(IP*) &getAddress()->sin6_addr = ip;
    ::inet_ntop(getAddress()->sin6_family, &getAddress()->sin6_addr, mIP, sizeof(mIP));
#else
    APP_ASSERT(sizeof(ip) == sizeof(getAddress()->sin_addr));
    *(IP*) &getAddress()->sin_addr = ip;
    ::inet_ntop(getAddress()->sin_family, &getAddress()->sin_addr, mIP, sizeof(mIP));
#endif
    mergeIP();
}


void CNetAddress::setPort(u16 port) {
    if(port != mPort) {
        mPort = port;
        initPort();
    }
}


u16 CNetAddress::getPort()const {
    return mPort; // getAddress()->sin_port;
}


void CNetAddress::set(const s8* ip, u16 port) {
    setIP(ip);
    setPort(port);
}

//TODO>>IPV6 decode
void CNetAddress::setIPort(const s8* ipAndPort) {
    if(ipAndPort == nullptr) {
        return;
    }
    IP it;
    convertStringToIP(ipAndPort, it);
    setIP(it);
    //if(*ipAndPort == '[') {//IPV6
    //}
    while(*ipAndPort != ':' && *ipAndPort != '\0') {
        ++ipAndPort;
    }
    if(*ipAndPort == ':') {
        setPort((u16) core::strtoul10(++ipAndPort));
    }
}

void CNetAddress::setDomain(const s8* iDNS) {
    if(!iDNS) {
        return;
    }

    /*
    LPHOSTENT hostTent = ::gethostbyname(iDNS);
    if(!hostTent) {
        return;
    }
    getAddress()->sin_addr = *((LPIN_ADDR) (*hostTent->h_addr_list));
    ::inet_ntop(getAddress()->sin_family, &getAddress()->sin_addr, mIP, sizeof(mIP));
    */

    struct addrinfo* head = 0;
    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
#if defined(APP_NET_USE_IPV6)
    hints.ai_family = getAddress()->sin6_family;
#else
    hints.ai_family = getAddress()->sin_family;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if(::getaddrinfo(iDNS, 0, &hints, &head)) {
        return;
    }

    for(struct addrinfo* curr = head; curr; curr = curr->ai_next) {
#if defined(APP_NET_USE_IPV6)
        if(curr->ai_family == getAddress()->sin6_family) {
            struct sockaddr_in6* sockaddr = (struct sockaddr_in6*) curr->ai_addr;
            getAddress()->sin6_addr = sockaddr->sin6_addr;
            ::inet_ntop(getAddress()->sin6_family, &getAddress()->sin6_addr, mIP, sizeof(mIP));
            mergeIP();
            //APP_LOG(ELOG_INFO, "CCollector::getLocalIP", "IPV6 = %s", mIP.c_str());
            break;
        }
#else
        if(curr->ai_family == getAddress()->sin_family) {
            struct sockaddr_in* sockaddr = (struct sockaddr_in*) curr->ai_addr;
            getAddress()->sin_addr = sockaddr->sin_addr;
            ::inet_ntop(getAddress()->sin_family, &getAddress()->sin_addr, mIP, sizeof(mIP));
            mergeIP();
            //APP_LOG(ELOG_INFO, "CCollector::getLocalIP", "IPV4 = %s", mIP.c_str());
            break;
        }
#endif
    } //for

    ::freeaddrinfo(head);
}


u16 CNetAddress::getFamily()const {
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(sizeof(u16) == sizeof(getAddress()->sin6_family));
    return (u16) getAddress()->sin6_family;
#else
    APP_ASSERT(sizeof(u16) == sizeof(getAddress()->sin_family));
    return (u16) getAddress()->sin_family;
#endif
}


APP_INLINE void CNetAddress::initIP() {
#if defined(APP_NET_USE_IPV6)
    ::inet_pton(getAddress()->sin6_family, mIP, &(getAddress()->sin6_addr));
#else
    //APP_ASSERT(getAddress());
    //getAddress()->sin_addr.S_un.S_addr = inet_addr(mIP.c_str());
    ::inet_pton(getAddress()->sin_family, mIP, &(getAddress()->sin_addr));
#endif
    mergeIP();
}

APP_INLINE void CNetAddress::mergeIP() {
    u8* pos = (u8*) (&mID);
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(sizeof(IP) == sizeof(getAddress()->sin6_addr));
    ::memcpy(pos, (&getAddress()->sin6_addr), sizeof(IP)); //128bit
#else
    APP_ASSERT(sizeof(IP) == sizeof(getAddress()->sin_addr));
    *((IP*) pos) = (*(IP*) (&getAddress()->sin_addr)); //32bit
#endif
}

APP_INLINE void CNetAddress::initPort() {
#if defined(APP_NET_USE_IPV6)
    getAddress()->sin6_port = htons(mPort);
#else
    getAddress()->sin_port = htons(mPort);
#endif
    mergePort();
}

APP_INLINE void CNetAddress::mergePort() {
    u8* pos = (u8*) (&mID);
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(sizeof(u16) == sizeof(getAddress()->sin6_port));
    APP_ASSERT(sizeof(IP) == sizeof(getAddress()->sin6_addr));
    pos += sizeof(IP);
    *((u16*) pos) = (*(u16*) (&getAddress()->sin6_port));
#else
    APP_ASSERT(sizeof(u16) == sizeof(getAddress()->sin_port));
    APP_ASSERT(sizeof(IP) == sizeof(getAddress()->sin_addr));
    pos += sizeof(IP);
    *((u16*) pos) = (*(u16*) (&getAddress()->sin_port));
#endif
}


#if defined(APP_NET_USE_IPV6)
void CNetAddress::setAddress(const sockaddr_in6& it) {
#else
void CNetAddress::setAddress(const sockaddr_in& it) {
#endif
    *getAddress() = it;
    reverse();
}


void CNetAddress::reverse() {
#if defined(APP_NET_USE_IPV6)
    ::inet_ntop(getAddress()->sin6_family, &getAddress()->sin6_addr, mIP, sizeof(mIP));
    mPort = ntohs(getAddress()->sin6_port);
#else
    ::inet_ntop(getAddress()->sin_family, &getAddress()->sin_addr, mIP, sizeof(mIP));
    mPort = ntohs(getAddress()->sin_port);
#endif
    mergeIP();
    mergePort();
}


const CNetAddress::IP& CNetAddress::getIP() const {
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(sizeof(IP) == sizeof(getAddress()->sin6_addr));
    return *(IP*) (&getAddress()->sin6_addr);
#else
    APP_ASSERT(sizeof(IP) == sizeof(getAddress()->sin_addr));
    return *(IP*) (&getAddress()->sin_addr);
#endif
}


//big endian
void CNetAddress::convertStringToIP(const s8* buffer, CNetAddress::IP& result) {
    APP_ASSERT(buffer);
#if defined(APP_NET_USE_IPV6)
    ::inet_pton(AF_INET6, buffer, &result);
#else
    const s8* end;
    result = core::strtoul10(buffer, &end);
    buffer = 1 + end;
    result |= core::strtoul10(buffer, &end) << 8;
    buffer = 1 + end;
    result |= core::strtoul10(buffer, &end) << 16;
    buffer = 1 + end;
    result |= core::strtoul10(buffer, &end) << 24;
#endif
}


//big endian
void CNetAddress::convertIPToString(const CNetAddress::IP& ip, core::CString& result) {
    s8 cache[APP_IP_STRING_MAX_SIZE];
#if defined(APP_NET_USE_IPV6)
    ::inet_ntop(AF_INET6, &ip, cache, APP_IP_STRING_MAX_SIZE);
#else
    u8 a = (ip & 0xff000000) >> 24;
    u8 b = (ip & 0x00ff0000) >> 16;
    u8 c = (ip & 0x0000ff00) >> 8;
    u8 d = (ip & 0x000000ff);
    ::snprintf(cache, APP_IP_STRING_MAX_SIZE, "%d.%d.%d.%d", d, c, b, a);
#endif
    result = cache;
}


}//namespace net
}//namespace app



//int main(int argc, char** argv) {
//    app::net::CNetAddress addr(APP_NET_DEFAULT_IP, 9012);
//    app::u32 idp = addr.getIPAsID(APP_NET_DEFAULT_IP);
//    app::core::CString ip(addr.getIDAsIP(idp));
//    printf("id-ip = %u-%s\n", idp, ip.c_str());
//    app::IUtility::print((app::u8*) &idp, sizeof(idp));
//    printf("\n");
//    app::IUtility::print((app::u8*) &addr.getAddress()->sin_addr, sizeof(addr.getAddress()->sin_addr));
//    printf("\nid=%u\n", *(app::u32*) &addr.getAddress()->sin_addr);
//    return 0;
//}
