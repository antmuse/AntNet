#include "CNetAddress.h"
#include "fast_atof.h"

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


namespace irr {
namespace net {

APP_INLINE void CNetAddress::init() {
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(28 == sizeof(sockaddr_in6));
    mAddress = (sockaddr_in6*) mCache;
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET6;
#else
    APP_ASSERT(16 == sizeof(sockaddr_in));
    mAddress = (sockaddr_in*) mCache;
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
#endif
    //printf("sizeof(sockaddr_in)=%u\n", sizeof(sockaddr_in));
    //printf("sizeof(sockaddr_in6)=%u\n", sizeof(sockaddr_in6));
}


CNetAddress::CNetAddress() :
    mID(0),
    mAddress(0),
    mPort(APP_NET_ANY_PORT) {
    ::strcpy(mIP, APP_NET_ANY_IP);
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const c8* ip) :
    mID(0),
    mAddress(0),
    mPort(APP_NET_ANY_PORT) {
    if(ip) {
        ::memcpy(mIP, ip, core::min_<size_t>(APP_IP_STRING_MAX_SIZE - sizeof(c8), ::strlen(ip) + sizeof(c8)));
        mIP[APP_IP_STRING_MAX_SIZE - sizeof(c8)] = '\0';
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const core::stringc& ip) :
    mID(0),
    mAddress(0),
    mPort(APP_NET_ANY_PORT) {
    if(ip.size() < APP_IP_STRING_MAX_SIZE) {
        ::memcpy(mIP, ip.c_str(), ip.size() + sizeof(c8));
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(u16 port) :
    mID(0),
    mAddress(0),
    mPort(port) {
    ::strcpy(mIP, APP_NET_ANY_IP);
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const c8* ip, u16 port) :
    mID(0),
    mAddress(0),
    mPort(port) {
    if(ip) {
        ::memcpy(mIP, ip, core::min_<size_t>(APP_IP_STRING_MAX_SIZE - sizeof(c8), ::strlen(ip) + sizeof(c8)));
        mIP[APP_IP_STRING_MAX_SIZE - sizeof(c8)] = '\0';
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }

    init();
    initIP();
    initPort();
}


CNetAddress::~CNetAddress() {
    //delete mAddress;
}


CNetAddress::CNetAddress(const core::stringc& ip, u16 port) :
    mID(0),
    mAddress(0),
    mPort(port) {
    if(ip.size() < APP_IP_STRING_MAX_SIZE) {
        ::memcpy(mIP, ip.c_str(), ip.size() + sizeof(c8));
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
    init();
    initIP();
    initPort();
}


CNetAddress::CNetAddress(const CNetAddress& other) {
#if defined(APP_NET_USE_IPV6)
    APP_ASSERT(28 == sizeof(sockaddr_in6));
    mAddress = (sockaddr_in6*) mCache;
#else
    APP_ASSERT(16 == sizeof(sockaddr_in));
    mAddress = (sockaddr_in*) mCache;
#endif
    ::memcpy(mAddress, other.mAddress, sizeof(*mAddress));
    ::memcpy(mIP, other.mIP, sizeof(mIP));
    mPort = other.mPort;
    mID = other.mID;
}


CNetAddress& CNetAddress::operator=(const CNetAddress& other) {
    if(this == &other) {
        return *this;
    }
    ::memcpy(mAddress, other.mAddress, sizeof(*mAddress));
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


void CNetAddress::setIP(const c8* ip) {
    if(ip) {
        ::memcpy(mIP, ip, core::min_<size_t>(APP_IP_STRING_MAX_SIZE - sizeof(c8), ::strlen(ip) + sizeof(c8)));
        mIP[APP_IP_STRING_MAX_SIZE - sizeof(c8)] = '\0';
        initIP();
    } else {
        ::strcpy(mIP, APP_NET_ANY_IP);
    }
}


bool CNetAddress::setIP() {
    c8 host_name[256];
    if(::gethostname(host_name, sizeof(host_name) - 1) < 0) {
        return false;
    }
    struct addrinfo* head = 0;
    struct addrinfo hints;
    hints.ai_family = mAddress->sin_family;
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
        if(curr->ai_family == mAddress->sin_family) {
            struct sockaddr_in* sockaddr = (struct sockaddr_in*) curr->ai_addr;
            mAddress->sin_addr = sockaddr->sin_addr;
            ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, mIP, sizeof(mIP));
            mergeIP();
            //APP_LOG(ELOG_INFO, "CCollector::getLocalIP", "IPV4 = %s", mIP.c_str());
            break;
        }
    } //for

    ::freeaddrinfo(head);
    return true;
}


void CNetAddress::setIP(const IP& ip) {
    APP_ASSERT(sizeof(ip) == sizeof(mAddress->sin_addr));
    *(IP*) &mAddress->sin_addr = ip;
    ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, mIP, sizeof(mIP));
    mergeIP();
}


void CNetAddress::setPort(u16 port) {
    if(port != mPort) {
        mPort = port;
        initPort();
    }
}


u16 CNetAddress::getPort()const {
    return mPort; // mAddress->sin_port;
}


void CNetAddress::set(const c8* ip, u16 port) {
    setIP(ip);
    setPort(port);
}


void CNetAddress::setDomain(const c8* iDNS) {
    if(!iDNS) {
        return;
    }

    /*
    LPHOSTENT hostTent = ::gethostbyname(iDNS);
    if(!hostTent) {
        return;
    }
    mAddress->sin_addr = *((LPIN_ADDR) (*hostTent->h_addr_list));
    ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, mIP, sizeof(mIP));
    */

    struct addrinfo* head = 0;
    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_family = mAddress->sin_family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if(::getaddrinfo(iDNS, 0, &hints, &head)) {
        return;
    }

    for(struct addrinfo* curr = head; curr; curr = curr->ai_next) {
        if(curr->ai_family == mAddress->sin_family) {
            struct sockaddr_in* sockaddr = (struct sockaddr_in*) curr->ai_addr;
            mAddress->sin_addr = sockaddr->sin_addr;
            ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, mIP, sizeof(mIP));
            mergeIP();
            //APP_LOG(ELOG_INFO, "CCollector::getLocalIP", "IPV4 = %s", mIP.c_str());
            break;
        }
    } //for

    ::freeaddrinfo(head);
}


u16 CNetAddress::getFamily()const {
    APP_ASSERT(sizeof(u16) == sizeof(mAddress->sin_family));
    return (u16) mAddress->sin_family;
}


APP_INLINE void CNetAddress::initIP() {
    //APP_ASSERT(mAddress);
    //mAddress->sin_addr.S_un.S_addr = inet_addr(mIP.c_str());
    ::inet_pton(mAddress->sin_family, mIP, &(mAddress->sin_addr));
    mergeIP();
}

APP_INLINE void CNetAddress::mergeIP() {
    APP_ASSERT(sizeof(IP) == sizeof(mAddress->sin_addr));
    u8* pos = (u8*) (&mID);
#if defined(APP_NET_USE_IPV6)
    ::memcpy(pos, (&mAddress->sin_addr), sizeof(IP)); //128bit
#else
    *((IP*) pos) = (*(IP*) (&mAddress->sin_addr)); //32bit
#endif
}

APP_INLINE void CNetAddress::initPort() {
    mAddress->sin_port = htons(mPort);
    mergePort();
}

APP_INLINE void CNetAddress::mergePort() {
    u8* pos = (u8*) (&mID);
    APP_ASSERT(sizeof(u16) == sizeof(mAddress->sin_port));
    APP_ASSERT(sizeof(IP) == sizeof(mAddress->sin_addr));
    pos += sizeof(IP);
    *((u16*) pos) = (*(u16*) (&mAddress->sin_port));
}


#if defined(APP_NET_USE_IPV6)
void CNetAddress::setAddress(const sockaddr_in6& it) {
#else
void CNetAddress::setAddress(const sockaddr_in& it) {
#endif
    *mAddress = it;
    reverse();
}


void CNetAddress::reverse() {
    ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, mIP, sizeof(mIP));
    mPort = ntohs(mAddress->sin_port);
    mergeIP();
    mergePort();
}


const CNetAddress::IP& CNetAddress::getIP() const {
    APP_ASSERT(sizeof(IP) == sizeof(mAddress->sin_addr));
    return *(IP*) (&mAddress->sin_addr);
}


//big endian
void CNetAddress::convertStringToIP(const c8* buffer, CNetAddress::IP& result) {
    APP_ASSERT(buffer);
#if defined(APP_NET_USE_IPV6)
    TODO >>
        const c8* end;
    IP ret = core::strtoul10(buffer, &end);
#else
    const c8* end;
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
void CNetAddress::convertIPToString(const CNetAddress::IP& ip, core::stringc& result) {
#if defined(APP_NET_USE_IPV6)
    TODO >>
#else
    u8 a = (ip & 0xff000000) >> 24;
    u8 b = (ip & 0x00ff0000) >> 16;
    u8 c = (ip & 0x0000ff00) >> 8;
    u8 d = (ip & 0x000000ff);
    c8 cache[APP_IP_STRING_MAX_SIZE];
    ::snprintf(cache, 40, "%d.%d.%d.%d", d, c, b, a);
    result = cache;
#endif
}


}//namespace net
}//namespace irr



//int main(int argc, char** argv) {
//    irr::net::CNetAddress addr(APP_NET_DEFAULT_IP, 9012);
//    irr::u32 idp = addr.getIPAsID(APP_NET_DEFAULT_IP);
//    irr::core::stringc ip(addr.getIDAsIP(idp));
//    printf("id-ip = %u-%s\n", idp, ip.c_str());
//    irr::IUtility::print((irr::u8*) &idp, sizeof(idp));
//    printf("\n");
//    irr::IUtility::print((irr::u8*) &addr.mAddress->sin_addr, sizeof(addr.mAddress->sin_addr));
//    printf("\nid=%u\n", *(irr::u32*) &addr.mAddress->sin_addr);
//    return 0;
//}
