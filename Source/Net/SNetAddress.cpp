#include "SNetAddress.h"
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
#endif  //APP_PLATFORM_WINDOWS


namespace irr {
namespace net {

SNetAddress::SNetAddress() :
    mID(0),
    mAddress(0),
    mIP(APP_NET_ANY_IP),
    mPort(APP_NET_ANY_PORT) {
    mAddress = new sockaddr_in();
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
    initIP();
    initPort();
}


SNetAddress::SNetAddress(const c8* ip) :
    mID(0),
    mAddress(0),
    mIP(ip),
    mPort(APP_NET_ANY_PORT) {
    mAddress = new sockaddr_in();
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
    initIP();
    initPort();
}


SNetAddress::SNetAddress(const core::stringc& ip) :
    mID(0),
    mAddress(0),
    mIP(ip),
    mPort(APP_NET_ANY_PORT) {
    mAddress = new sockaddr_in();
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
    initIP();
    initPort();
}


SNetAddress::SNetAddress(u16 port) :
    mID(0),
    mAddress(0),
    mIP(APP_NET_ANY_IP),
    mPort(port) {
    mAddress = new sockaddr_in();
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
    initIP();
    initPort();
}


SNetAddress::~SNetAddress() {
    delete mAddress;
}


SNetAddress::SNetAddress(const c8* ip, u16 port) :
    mID(0),
    mAddress(0),
    mIP(ip),
    mPort(port) {
    mAddress = new sockaddr_in();
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
    initIP();
    initPort();
}


SNetAddress::SNetAddress(const core::stringc& ip, u16 port) :
    mID(0),
    mAddress(0),
    mIP(ip),
    mPort(port) {
    mAddress = new sockaddr_in();
    ::memset(mAddress, 0, sizeof(*mAddress));
    mAddress->sin_family = AF_INET;
    initIP();
    initPort();
}


SNetAddress::SNetAddress(const SNetAddress& other) {
    mAddress = new sockaddr_in();
    ::memcpy(mAddress, other.mAddress, sizeof(*mAddress));
    mIP = other.mIP;
    mPort = other.mPort;
    mID = other.mID;
}


SNetAddress& SNetAddress::operator=(const SNetAddress& other) {
    if(this == &other) {
        return *this;
    }
    if(0 == mAddress) {
        mAddress = new sockaddr_in();
        mAddress->sin_family = AF_INET;
    }
    ::memcpy(mAddress, other.mAddress, sizeof(*mAddress));
    mID = other.mID;
    mIP = other.mIP;
    mPort = other.mPort;
    return *this;
}


bool SNetAddress::operator==(const SNetAddress& other) const {
    return (mID == other.mID);
}


bool SNetAddress::operator!=(const SNetAddress& other) const {
    return (mID != other.mID);
}


void SNetAddress::setIP(const c8* ip) {
    if(ip) {
        mIP = ip;
        initIP();
    }
}


void SNetAddress::setPort(u16 port) {
    if(port != mPort) {
        mPort = port;
        initPort();
    }
}


void SNetAddress::set(const c8* ip, u16 port) {
    if(ip) {
        mIP = ip;
        initIP();
    }
    mPort = port;
    initPort();
}


void SNetAddress::setDomain(const c8* iDNS) {
    if(!iDNS) {
        return;
    }
    /*LPHOSTENT hostTent = ::gethostbyname(iDNS);
    if(!hostTent) {
        return;
    }

    c8 buf[40];
    mAddress->sin_addr = *((LPIN_ADDR) (*hostTent->h_addr_list));
    ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, buf, sizeof(buf));
    mIP = buf;*/
#if defined(APP_PLATFORM_WINDOWS)

    struct addrinfo* head = 0;
    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_family = mAddress->sin_family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if(::getaddrinfo(iDNS, 0, &hints, &head)) {
        return;
    }

    c8 buf[40];

    for(struct addrinfo* curr = head; curr; curr = curr->ai_next) {
        if(curr->ai_family == mAddress->sin_family) {
            struct sockaddr_in* sockaddr = (struct sockaddr_in*) curr->ai_addr;
            mAddress->sin_addr = sockaddr->sin_addr;
            ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, buf, sizeof(buf));
            mIP = buf;
            mergeIP();
            //APP_LOG(ELOG_INFO, "CCollector::getLocalIP", "IPV4 = %s", mIP.c_str());
            break;
        }
    } //for

    ::freeaddrinfo(head);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)

#endif
}


u16 SNetAddress::getFamily()const {
    APP_ASSERT(sizeof(u16) == sizeof(mAddress->sin_family));
    return (u16) mAddress->sin_family;
}


APP_INLINE void SNetAddress::initIP() {
    //APP_ASSERT(mAddress);
    //mAddress->sin_addr.S_un.S_addr = inet_addr(mIP.c_str());
    ::inet_pton(mAddress->sin_family, mIP.c_str(), &(mAddress->sin_addr));
    mergeIP();
}

APP_INLINE void SNetAddress::mergeIP() {
    APP_ASSERT(sizeof(IP) == sizeof(mAddress->sin_addr));
    u8* pos = (u8*) (&mID);
#if defined(APP_NET_USE_IPV6)
    ::memcpy(pos, (&mAddress->sin_addr), sizeof(IP)); //128bit
#else
    *((IP*) pos) = (*(IP*) (&mAddress->sin_addr)); //32bit
#endif
}

APP_INLINE void SNetAddress::initPort() {
    mAddress->sin_port = ::htons(mPort);
    mergePort();
}

APP_INLINE void SNetAddress::mergePort() {
    u8* pos = (u8*) (&mID);
    APP_ASSERT(sizeof(u16) == sizeof(mAddress->sin_port));
    APP_ASSERT(sizeof(IP) == sizeof(mAddress->sin_addr));
    pos += sizeof(IP);
    *((u16*) pos) = (*(u16*) (&mAddress->sin_port));
}


void SNetAddress::reverse() {
    c8 buf[40]; //40=32+8, ipv6 as "CDCD:910A:2222:5498:8475:1111:3900:2020"
    ::inet_ntop(mAddress->sin_family, &mAddress->sin_addr, buf, sizeof(buf));
    mIP = buf;
    mPort = ::ntohs(mAddress->sin_port);
    mergeIP();
    mergePort();
}


const SNetAddress::IP& SNetAddress::getIPAsID() const {
    APP_ASSERT(sizeof(IP) == sizeof(mAddress->sin_addr));
    return *(IP*)(&mAddress->sin_addr);
}


//big endian
SNetAddress::IP SNetAddress::getIPAsID(const c8* buffer) {
#if defined(APP_NET_USE_IPV6)
    TODO>>
    const c8* end;
    IP ret = core::strtoul10(buffer, &end);
    return ret;
#else
    const c8* end;
    IP ret = core::strtoul10(buffer, &end);
    buffer = 1 + end;
    ret |= core::strtoul10(buffer, &end) << 8;
    buffer = 1 + end;
    ret |= core::strtoul10(buffer, &end) << 16;
    buffer = 1 + end;
    ret |= core::strtoul10(buffer, &end) << 24;
    return ret;
#endif
}


//big endian
core::stringc SNetAddress::getIDAsIP(SNetAddress::IP& ip) {
#if defined(APP_NET_USE_IPV6)
    TODO >>
#else
    u8 a = (ip & 0xff000000) >> 24;
    u8 b = (ip & 0x00ff0000) >> 16;
    u8 c = (ip & 0x0000ff00) >> 8;
    u8 d = (ip & 0x000000ff);
    c8 cache[40];
    ::snprintf(cache, 40, "%d.%d.%d.%d", d, c, b, a);
    return core::stringc(cache);
#endif
}


}//namespace net
}//namespace irr



//int main(int argc, char** argv) {
//    irr::net::SNetAddress addr(APP_NET_DEFAULT_IP, 9012);
//    irr::u32 idp = addr.getIPAsID(APP_NET_DEFAULT_IP);
//    irr::core::stringc ip(addr.getIDAsIP(idp));
//    printf("id-ip = %u-%s\n", idp, ip.c_str());
//    irr::IUtility::print((irr::u8*) &idp, sizeof(idp));
//    printf("\n");
//    irr::IUtility::print((irr::u8*) &addr.mAddress->sin_addr, sizeof(addr.mAddress->sin_addr));
//    printf("\nid=%u\n", *(irr::u32*) &addr.mAddress->sin_addr);
//    return 0;
//}