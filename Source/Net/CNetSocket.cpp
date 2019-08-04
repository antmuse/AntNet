#include "CNetSocket.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include "SClientContext.h"
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/sockios.h>
#endif  //APP_PLATFORM_WINDOWS

#if defined(APP_NET_USE_IPV6)
#define APP_IP_VERSION AF_INET6
#else
#define APP_IP_VERSION AF_INET
#endif


namespace irr {
namespace net {

CNetSocket::CNetSocket(netsocket sock) :
    mSocket(sock) {
    APP_ASSERT(sizeof(socklen_t) == sizeof(u32));
}


CNetSocket::CNetSocket() :
    mSocket(APP_INVALID_SOCKET) {
#if defined(APP_PLATFORM_WINDOWS)
    APP_ASSERT(INVALID_SOCKET == APP_INVALID_SOCKET);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    //APP_ASSERT(-1 == APP_INVALID_SOCKET);
#endif
}


CNetSocket::~CNetSocket() {
}


bool CNetSocket::close() {
    if(APP_INVALID_SOCKET == mSocket) {
        return false;
    }

#if defined(APP_PLATFORM_WINDOWS)
    if(::closesocket(mSocket)) {
        return false;
    }
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    if(::close(mSocket)) {
        return false;
    }
#endif  //APP_PLATFORM_WINDOWS

    mSocket = APP_INVALID_SOCKET;
    return true;
}


bool CNetSocket::closeReceive() {
    return 0 == ::shutdown(mSocket, ESHUT_RECEIVE);
}

bool CNetSocket::closeSend() {
    return 0 == ::shutdown(mSocket, ESHUT_SEND);
}

bool CNetSocket::closeBoth() {
    return 0 == ::shutdown(mSocket, ESHUT_BOTH);
}


bool CNetSocket::isOpen()const {
    return APP_INVALID_SOCKET != mSocket;
}


s32 CNetSocket::getError() {
#if defined(APP_PLATFORM_WINDOWS)
    return ::WSAGetLastError();  // GetLastError();
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return errno;
#endif
}


void CNetSocket::getLocalAddress(CNetAddress& it) {
    socklen_t len = sizeof(*it.getAddress());
    ::getsockname(mSocket, (struct sockaddr*)it.getAddress(), &len);
    it.reverse();
    //APP_LOG(ELOG_DEBUG, "CNetSocket::getLocalAddress", "local: [%s:%d]", it.mIP.c_str(), it.mPort);
}


void CNetSocket::getRemoteAddress(CNetAddress& it) {
    socklen_t len = sizeof(*it.getAddress());
    ::getpeername(mSocket, (struct sockaddr*)it.getAddress(), &len);
    it.reverse();
    //APP_LOG(ELOG_DEBUG, "CNetSocket::getRemoteAddress", "remote: [%s:%d]", it.mIP.c_str(), it.mPort);
}


s32 CNetSocket::setReceiveOvertime(u32 it) {
#if defined(APP_PLATFORM_WINDOWS)
    return ::setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (c8*) &it, sizeof(it));
#else
    struct timeval time;
    time.tv_sec = it / 1000;                     //seconds
    time.tv_usec = 1000 * (it % 1000);         //microseconds
    return ::setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (c8*) &time, sizeof(time));
#endif
}


s32 CNetSocket::setSendOvertime(u32 it) {
#if defined(APP_PLATFORM_WINDOWS)
    return ::setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, (c8*) &it, sizeof(it));
#else
    struct timeval time;
    time.tv_sec = it / 1000;                     //seconds
    time.tv_usec = 1000 * (it % 1000);           //microseconds
    return ::setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, (c8*) &time, sizeof(time));
#endif
}


s32 CNetSocket::setCustomIPHead(bool on) {
    s32 opt = on ? 1 : 0;
    return ::setsockopt(mSocket, IPPROTO_IP, IP_HDRINCL, (c8*) &opt, sizeof(opt));
}


s32 CNetSocket::setReceiveAll(bool on) {
#if defined(APP_PLATFORM_WINDOWS)
    u32 dwBufferLen[10];
    u32 dwBufferInLen = on ? RCVALL_ON : RCVALL_OFF;
    DWORD dwBytesReturned = 0;
    //::ioctlsocket(mScoketListener.getValue(), SIO_RCVALL, &dwBufferInLen);
    return (::WSAIoctl(mSocket, SIO_RCVALL,
        &dwBufferInLen, sizeof(dwBufferInLen),
        &dwBufferLen, sizeof(dwBufferLen),
        &dwBytesReturned, 0, 0));
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    ifreq iface;
    char* device = "eth0";
    APP_ASSERT(IFNAMSIZ > ::strlen(device));

    s32 setopt_result = ::setsockopt(mSocket, SOL_SOCKET,
        SO_BINDTODEVICE, device, ::strlen(device));

    ::strcpy(iface.ifr_name, device);
    if(::ioctl(mSocket, SIOCGIFFLAGS, &iface) < 0) {
        return -1;
    }
    if(!(iface.ifr_flags & IFF_RUNNING)) {
        //printf("eth link down\n");
        return -1;
    }
    if(on) {
        iface.ifr_flags |= IFF_PROMISC;
    } else {
        iface.ifr_flags &= ~IFF_PROMISC;
    }
    return ::ioctl(mSocket, SIOCSIFFLAGS, &iface);
#endif
}


s32 CNetSocket::keepAlive(bool on, s32 idleTime, s32 timeInterval, s32 maxTick) {
#if defined(APP_PLATFORM_WINDOWS)
    //struct tcp_keepalive {
    //    ULONG onoff;   // 是否开启 keepalive
    //    ULONG keepalivetime;  // 多长时间（ ms ）没有数据就开始 send 心跳包
    //    ULONG keepaliveinterval; // 每隔多长时间（ ms ） send 一个心跳包，
    //   // 发 5 次 (2000 XP 2003 默认 ), 10 次 (Vista 后系统默认 )
    //};
    tcp_keepalive alive;
    DWORD rbytes;
    alive.onoff = on ? TRUE : FALSE;
    alive.keepalivetime = 1000 * idleTime;
    alive.keepaliveinterval = 1000 * timeInterval;
    return ::WSAIoctl(mSocket, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), 0, 0, &rbytes, 0, 0);

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    s32 opt = on ? 1 : 0;
    s32 ret;
    //SOL_TCP
    ret = ::setsockopt(mSocket, SOL_SOCKET, SO_KEEPALIVE, (c8*) &opt, sizeof(opt));
    if(!on || ret) return ret;

    ret = ::setsockopt(mSocket, SOL_SOCKET, TCP_KEEPIDLE, &idleTime, sizeof(idleTime));
    if(ret) return ret;

    ret = ::setsockopt(mSocket, SOL_SOCKET, TCP_KEEPINTVL, &timeInterval, sizeof(timeInterval));
    if(ret) return ret;

    ret = ::setsockopt(mSocket, SOL_SOCKET, TCP_KEEPCNT, &maxTick, sizeof(maxTick));

    return ret;
#endif
}


s32 CNetSocket::setLinger(bool it, u16 iTime) {
    struct linger val;
    val.l_onoff = it ? 1 : 0;
    val.l_linger = iTime;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (c8*) &val, sizeof(val));
}


s32 CNetSocket::setBroadcast(bool it) {
    s32 opt = it ? 1 : 0;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, (c8*) &opt, sizeof(opt));
}


s32 CNetSocket::setBlock(bool it) {
#if defined(APP_PLATFORM_WINDOWS)
    u_long ul = it ? 0 : 1;
    return ::ioctlsocket(mSocket, FIONBIO, &ul);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    s32 opts = ::fcntl(mSocket, F_GETFL);
    if(opts < 0) {
        return -1;
    }
    opts = it ? (opts & (~O_NONBLOCK)) : opts | O_NONBLOCK;
    opts = ::fcntl(mSocket, F_SETFL, opts);
    return -1 == opts ? opts : 0;
#endif
}


s32 CNetSocket::setReuseIP(bool it) {
    s32 opt = it ? 1 : 0;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (c8*) &opt, sizeof(opt));
}


s32 CNetSocket::setReusePort(bool it) {
#if defined(APP_PLATFORM_WINDOWS)
    return 0;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    s32 opt = it ? 1 : 0;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_REUSEPORT, (c8*) &opt, sizeof(opt));
#endif
}


s32 CNetSocket::setSendCache(s32 size) {
    return ::setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (c8*) &size, sizeof(size));
}


s32 CNetSocket::setReceiveCache(s32 size) {
    return ::setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (c8*) &size, sizeof(size));
}


s32 CNetSocket::bind(const CNetAddress& it) {
    return ::bind(mSocket, (sockaddr*) it.getAddress(), sizeof(*it.getAddress()));
}


s32 CNetSocket::bind() {
    CNetAddress it;
    return ::bind(mSocket, (sockaddr*) it.getAddress(), sizeof(*it.getAddress()));
}

bool CNetSocket::getTcpInfo(STCP_Info* info) const {
    //TODO>>
#if defined(APP_PLATFORM_WINDOWS)
    bool ret = false;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    struct tcp_info raw;
    socklen_t len = sizeof(raw);
    ::memset(&raw, 0, len);
    bool ret = (0 == ::getsockopt(mSocket, SOL_SOCKET, TCP_INFO, &raw, &len));
#endif
    return ret;
}


s32 CNetSocket::sendto(const void* iBuffer, s32 iSize, const CNetAddress& address) {
#if defined(APP_PLATFORM_WINDOWS)
    return ::sendto(mSocket, (const c8*) iBuffer, iSize, 0,
        (sockaddr*) address.getAddress(), sizeof(*address.getAddress()));
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return ::sendto(mSocket, iBuffer, iSize, 0,
        (sockaddr*) address.getAddress(), sizeof(*address.getAddress()));
#endif
}


s32 CNetSocket::receiveFrom(void* iBuffer, s32 iSize, const CNetAddress& address) {
#if defined(APP_PLATFORM_WINDOWS)
    socklen_t size = sizeof(*address.getAddress());
    return ::recvfrom(mSocket, (c8*) iBuffer, iSize, 0, (sockaddr*) address.getAddress(), &size);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    socklen_t size = sizeof(*address.getAddress());
    return ::recvfrom(mSocket, iBuffer, iSize, 0, (sockaddr*) address.getAddress(), &size);
#endif
}


bool CNetSocket::openRaw(s32 protocol) {
    return open(APP_IP_VERSION, SOCK_RAW, protocol);
}


bool CNetSocket::openUDP() {
    return open(APP_IP_VERSION, SOCK_DGRAM, IPPROTO_UDP);
}


bool CNetSocket::openTCP() {
    return open(APP_IP_VERSION, SOCK_STREAM, IPPROTO_TCP);
}


bool CNetSocket::open(s32 domain, s32 type, s32 protocol) {
    if(APP_INVALID_SOCKET != mSocket) {
        return false;
    }

    mSocket = ::socket(domain, type, protocol);
    return APP_INVALID_SOCKET != mSocket;
}



s32 CNetSocket::setDelay(bool it) {
    s32 opt = it ? 0 : 1;
    return ::setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (c8*) &opt, sizeof(opt));
}


s32 CNetSocket::connect(const CNetAddress& it) {
    return ::connect(mSocket, (sockaddr*) it.getAddress(), sizeof(*it.getAddress()));
}


s32 CNetSocket::listen(u32 max) {
    return ::listen(mSocket, max);
}


bool CNetSocket::isAlive() {
    return (0 == send("", 0));
}


s32 CNetSocket::sendAll(const void* iBuffer, s32 iSize) {
    s32 ret = 0;
    s32 step;
    for(; ret < iSize;) {
        step = send(((const c8*) iBuffer) + ret, iSize - ret);
        if(step > 0) {
            ret += step;
        } else {
            return step;
        }
    }
    return ret;
}


s32 CNetSocket::send(const void* iBuffer, s32 iSize) {
#if defined(APP_PLATFORM_WINDOWS)
    return ::send(mSocket, (const c8*) iBuffer, iSize, 0);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return ::send(mSocket, iBuffer, iSize, MSG_NOSIGNAL);
#endif
}


s32 CNetSocket::receiveAll(void* iBuffer, s32 iSize) {
    s32 ret = 0;
    s32 step;
    for(; ret < iSize;) {
        step = ::recv(mSocket, ((c8*) iBuffer) + ret, iSize - ret, MSG_WAITALL);
        if(step > 0) {
            ret += step;
        } else {
            return step;
        }
    }
    return ret;
//#if defined(APP_PLATFORM_WINDOWS)
//    return ::recv(mSocket, (c8*) iBuffer, iSize, MSG_WAITALL);
//#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
//    return ::recv(mSocket, iBuffer, iSize, MSG_WAITALL);
//#endif
}


s32 CNetSocket::receive(void* iBuffer, s32 iSize) {
#if defined(APP_PLATFORM_WINDOWS)
    return ::recv(mSocket, (c8*) iBuffer, iSize, 0);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return ::recv(mSocket, iBuffer, iSize, 0);
#endif
}


CNetSocket CNetSocket::accept() {
    return ::accept(mSocket, 0, 0);;
}


CNetSocket CNetSocket::accept(CNetAddress& it) {
#if defined(APP_PLATFORM_WINDOWS) || defined(APP_PLATFORM_ANDROID)
    s32 size = sizeof(*it.getAddress());
#elif defined(APP_PLATFORM_LINUX)
    u32 size = sizeof(*it.getAddress());
#endif
    return ::accept(mSocket, (sockaddr*) it.getAddress(), &size);
}



#if defined(APP_PLATFORM_WINDOWS)
s32 CNetSocket::updateByAccepter(const CNetSocket& sockListen) {
    netsocket it = sockListen.getValue();
    return ::setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (c8*) &it, sizeof(it));
}


bool CNetSocket::openSeniorTCP() {
    return openSenior(APP_IP_VERSION, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
}


bool CNetSocket::openSenior(s32 domain, s32 type, s32 protocol, void* info, u32 group, u32 flag) {
    if(APP_INVALID_SOCKET != mSocket) {
        return false;
    }
    mSocket = ::WSASocket(domain, type, protocol, (LPWSAPROTOCOL_INFOW) info, group, flag);
    return APP_INVALID_SOCKET != mSocket;
}


bool CNetSocket::receive(SContextIO* iAction) {
    APP_ASSERT(0 == iAction->mFlags);
    APP_ASSERT(0 == iAction->mBytes);
    s32 ret = ::WSARecv(mSocket,
        &(iAction->mBuffer),
        1,
        &(iAction->mBytes),
        &(iAction->mFlags),
        &(iAction->mOverlapped),
        0);

    return 0 != ret ? (WSA_IO_PENDING == CNetSocket::getError()) : true;
}


bool CNetSocket::send(SContextIO* iAction) {
    s32 ret = ::WSASend(mSocket,
        &(iAction->mBuffer),
        1,
        &(iAction->mBytes),
        (iAction->mFlags),
        &(iAction->mOverlapped),
        0);

    return 0 != ret ? (WSA_IO_PENDING == CNetSocket::getError()) : true;
}


bool CNetSocket::connect(const CNetAddress& it, SContextIO* iAction, void* function/* = 0*/) {
    LPFN_CONNECTEX iConnect = (LPFN_CONNECTEX) (function ? function : getFunctionConnect());
    if(iConnect) {
        if(FALSE == iConnect(mSocket,
            (sockaddr*) it.getAddress(),
            sizeof(*it.getAddress()),
            0,
            0,
            &iAction->mBytes,
            &iAction->mOverlapped)) {
            return (WSA_IO_PENDING == CNetSocket::getError());
        }
        return true;
    }

    return false;
}


bool CNetSocket::disconnect(SContextIO* iAction, void* function/* = 0*/) {
    LPFN_DISCONNECTEX iDisconnect = (LPFN_DISCONNECTEX) (function ? function : getFunctionDisconnect());
    if(iDisconnect) {
        if(FALSE == iDisconnect(mSocket, &iAction->mOverlapped, iAction->mFlags, 0)) {
            return (WSA_IO_PENDING == CNetSocket::getError());
        }
        return true;
    }

    return false;
}


bool CNetSocket::accept(const CNetSocket& sock, SContextIO* iAction, void* addressCache, void* function/* = 0*/) {
    LPFN_ACCEPTEX func = (LPFN_ACCEPTEX) (function ? function : getFunctionAccpet());
    if(func) {
#if defined(APP_NET_USE_IPV6)
        const u32 addsize = sizeof(sockaddr_in6) + 16;
#else
        const u32 addsize = sizeof(sockaddr_in) + 16;
#endif
        if(FALSE == func(mSocket,
            sock.getValue(),
            addressCache,
            0,
            addsize,
            addsize,
            &iAction->mBytes,
            &iAction->mOverlapped)) {
            return (WSA_IO_PENDING == CNetSocket::getError());
        }
        return true;
    }

    return false;
}


bool CNetSocket::getAddress(void* addressCache, CNetAddress& local, CNetAddress& remote, void* function/* = 0*/)const {
    LPFN_GETACCEPTEXSOCKADDRS func = (LPFN_GETACCEPTEXSOCKADDRS) (function ? function : getFunctionAcceptSockAddress());
    if(func) {
#if defined(APP_NET_USE_IPV6)
        const u32 addsize = sizeof(sockaddr_in6) + 16;
        sockaddr_in6* addrLocal;
        sockaddr_in6* addrRemote;
#else
        const u32 addsize = sizeof(sockaddr_in) + 16;
        sockaddr_in* addrLocal;
        sockaddr_in* addrRemote;
#endif
        s32 remoteLen = sizeof(*addrLocal);
        s32 localLen = sizeof(*addrRemote);
        func(addressCache,
            0, //2*addsize
            addsize,
            addsize,
            (sockaddr**) &addrLocal,
            &localLen,
            (sockaddr**) &addrRemote,
            &remoteLen);

        local.setAddress(*addrLocal);
        remote.setAddress(*addrRemote);
        return true;
    }

    return false;
}


void* CNetSocket::getFunctionConnect()const {
    LPFN_CONNECTEX func = 0;
    GUID iGID = WSAID_CONNECTEX;
    DWORD retBytes = 0;

    if(0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


void* CNetSocket::getFunctionDisconnect() const {
    LPFN_DISCONNECTEX func = 0;
    GUID iGID = WSAID_DISCONNECTEX;
    DWORD retBytes = 0;

    if(0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


void* CNetSocket::getFunctionAccpet()const {
    LPFN_ACCEPTEX func = 0;
    GUID iGID = WSAID_ACCEPTEX;
    DWORD retBytes = 0;

    if(0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


void* CNetSocket::getFunctionAcceptSockAddress()const {
    LPFN_GETACCEPTEXSOCKADDRS func = 0;
    GUID iGID = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD retBytes = 0;

    if(0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}

#endif//APP_PLATFORM_WINDOWS






#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
CNetSocketPair::CNetSocketPair() {
}

CNetSocketPair::~CNetSocketPair() {
}


bool CNetSocketPair::close() {
    return mSockB.close() && mSockA.close();
}

bool CNetSocketPair::open() {
    netsocket sockpair[2];
    if(::socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockpair)) {
        //printf("error %d on socketpair\n", errno);
        return false;
    }
    mSockA = sockpair[0];
    mSockB = sockpair[1];
    return true;
}


bool CNetSocketPair::open(s32 domain, s32 type, s32 protocol) {
    netsocket sockpair[2];
    if(::socketpair(domain, type, protocol, sockpair)) {
        //printf("error %d on socketpair\n", errno);
        return false;
    }
    mSockA = sockpair[0];
    mSockB = sockpair[1];
    u32 sz = 128 * sizeof(u32);
    mSockA.setSendCache(sz);
    mSockA.setReceiveCache(sz);
    mSockB.setSendCache(sz);
    mSockB.setReceiveCache(sz);
    return true;
}

#endif //OS APP_PLATFORM_LINUX  APP_PLATFORM_ANDROID
} //namespace net
} //namespace irr
