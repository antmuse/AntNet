#ifndef APP_CNETSOCKET_H
#define APP_CNETSOCKET_H


//#include "HConfig.h"
#include "HNetConfig.h"
#include "CNetAddress.h"

namespace irr {
namespace net {
struct SContextIO;

#define APP_INVALID_SOCKET   (-1)
#define APP_SOCKET_ERROR     (-1)


#if defined(APP_PLATFORM_WINDOWS)
#if defined(APP_OS_64BIT)
typedef irr::u64 netsocket;
#else
typedef irr::u32 netsocket;
#endif

enum ESystemErrorCode {
    ESEC_TIMEOUT = 10060,   //WSAETIMEDOUT
};
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
typedef irr::s32 netsocket;
enum ESystemErrorCode {
    ESEC_TIMEOUT = 110,     //ETIMEDOUT
};
#endif //APP_PLATFORM_WINDOWS


enum ENetErrorCode {
    ENEC_TIMEOUT
};



class CNetSocket {
public:
    enum EShutFlag {
        ESHUT_RECEIVE,
        ESHUT_SEND,
        ESHUT_BOTH
    };
    struct STCP_Info {
        //TODO
    };

    CNetSocket(netsocket sock);

    CNetSocket();

    ~CNetSocket();

    static s32 getError();

    CNetSocket(const CNetSocket& other) {
        mSocket = other.mSocket;
    }

    CNetSocket& operator=(const CNetSocket& other) {
        if(this == &other) {
            return *this;
        }
        mSocket = other.mSocket;
        return *this;
    }

    CNetSocket& operator=(const netsocket other) {
        mSocket = other;
        return *this;
    }

    bool operator==(const CNetSocket& other) const {
        return mSocket == other.mSocket;
    }

    bool operator!=(const CNetSocket& other) const {
        return mSocket != other.mSocket;
    }

    bool operator>(const CNetSocket& other) const {
        return mSocket > other.mSocket;
    }

    bool operator<(const CNetSocket& other) const {
        return mSocket < other.mSocket;
    }

    bool operator>=(const CNetSocket& other) const {
        return mSocket >= other.mSocket;
    }

    bool operator<=(const CNetSocket& other) const {
        return mSocket <= other.mSocket;
    }


    bool isOpen()const;


    netsocket getValue() const {
        return mSocket;
    }

    void setInvalid() {
        mSocket = APP_INVALID_SOCKET;
    }

    /**
    *@brief Set send overtime.
    *@param iTime The time in milliseconds.
    *@return 0 if successed, else failed.
    */
    s32 setSendOvertime(u32 iTime);

    /**
    *@brief Set receive overtime.
    *@param iTime The time in milliseconds.
    *@return 0 if successed, else failed.
    */
    s32 setReceiveOvertime(u32 iTime);

    /**
    *@brief Set keep alive.
    *@param on ture to turn on, else turn off.
    *@param idleTime How long to start tick when idle, in seconds.
    *@param timeInterval Time interval of tick, in seconds.
    *@param maxTick max ticks of heartbeat, ignored on windows OS.
    *@return 0 if successed, else failed.
    *@note On Windows Vista and later, the max tick is set to 10 and cannot be changed.
    */
    s32 keepAlive(bool on, s32 idleTime, s32 timeInterval, s32 maxTick);

    /**
    *@brief Set broadcast.
    *@return 0 if successed, else failed.
    */
    s32 setBroadcast(bool it);

    /**
    *@brief Set linger.
    *@param it true to turn on linger, else turn off.
    *@param iTime Time in seconds.
    *@return 0 if successed, else failed.
    */
    s32 setLinger(bool it, u16 iTime);

    /**
    *@brief Set block.
    *@return 0 if successed, else failed.
    */
    s32 setBlock(bool it);

    /**
    *@brief Set reuse IP.
    *@return 0 if successed, else failed.
    */
    s32 setReuseIP(bool it);

    /**
    *@brief Enbale or disable custom IP head, only raw socket.
    *@param on OS write IP head if true, else user write, default is true.
    *@return 0 if successed, else failed.
    */
    s32 setCustomIPHead(bool on);

    /**
    *@brief Set receive all data, promiscuous mode, only raw socket.
    *@return 0 if successed, else failed.
    */
    s32 setReceiveAll(bool on);

    /**
    *@brief Set reuse port.
    *@return 0 if successed, else failed.
    */
    s32 setReusePort(bool on);

    /**
    *@brief Set send cache size of socket.
    *@param size The socket cache size.
    *@return 0 if success, else if failed.
    */
    s32 setSendCache(s32 size);

    /**
    *@brief Set receive cache size of socket.
    *@param size The socket cache size.
    *@return 0 if success, else if failed.
    */
    s32 setReceiveCache(s32 size);

    /**
    *@brief Close socket.
    *@return true if success, else false.
    */
    bool close();
    bool closeSend();
    bool closeReceive();
    bool closeBoth();

    void getLocalAddress(CNetAddress& it);

    void getRemoteAddress(CNetAddress& it);

    //s32 getFamily()const;

    /**
    *@brief Bind socket with address.
    *@param it net address.
    *@return 0 if success, else if failed.
    */
    s32 bind(const CNetAddress& it);

    /**
    *@brief Bind socket with any address["0.0.0.0:0"].
    *@return 0 if success, else if failed.
    */
    s32 bind();

    bool getTcpInfo(STCP_Info* info) const;

    /**
    *@brief Open a raw socket.
    *@param protocol Net protocol.
    *@return true if success, else if false.
    */
    bool openRaw(s32 protocol);

    bool openTCP();

    bool openUDP();

    /**
    *@brief Open a user defined socket.
    *@return true if success, else if false.
    */
    bool open(s32 domain, s32 type, s32 protocol);


    s32 sendto(const void* iBuffer, s32 iSize, const CNetAddress& address);

    s32 receiveFrom(void* iBuffer, s32 iSize, const CNetAddress& address);


    s32 send(const void* iBuffer, s32 iSize);

    /**
    *@brief Send in a loop, if using nonblock socket.
    */
    s32 sendAll(const void* iBuffer, s32 iSize);

    s32 receive(void* iBuffer, s32 iSize);

    s32 receiveAll(void* iBuffer, s32 iSize);

    /**
    *@brief Set delay.
    *@param it true to enable nagle, else disable nagle.
    *@return 0 if successed, else failed.
    */
    s32 setDelay(bool it);


    /**
    *@brief Connect with peer.
    *@return 0 if successed, else failed.
    */
    s32 connect(const CNetAddress& it);

    /**
    *@brief Listen peer.
    *@return 0 if successed, else failed.
    */
    s32 listen(u32 max);

    /**
    *@brief Accpet a new client.
    *@return A valid socket if successed, else invalid.
    */
    CNetSocket accept();

    /**
    *@brief Accpet a new client.
    *@param it net address of remote peer.
    *@return A valid socket if successed, else invalid.
    */
    CNetSocket accept(CNetAddress& it);


    bool isAlive();


#if defined(APP_PLATFORM_WINDOWS)
    /**
    *@brief Update socket by accept socket.
    *@param sock accept socket.
    *@return 0 if success, else if failed.
    */
    s32 updateByAccepter(const CNetSocket& sock);

    /**
    * @brief Windows only
    */
    bool openSeniorTCP();

    /**
    * @brief Windows only
    */
    bool openSenior(s32 domain, s32 type, s32 protocol, void* info, u32 group, u32 flag);

    /**
    *@brief Post a accept action.
    *@param sock The accpet socket.
    *@param iAction The accpet action.
    *@param addressCache The cache to store addresses of local and remote.
    *@note: In IPV4, addressCache size must not less than 64 = 2*(sizeof(sockaddr_in) + 16).
    */
    bool accept(const CNetSocket& sock, SContextIO* iAction, void* addressCache, void* function = 0);

    bool getAddress(void* addressCache, CNetAddress& local, CNetAddress& remote, void* function = 0)const;

    bool connect(const CNetAddress& it, SContextIO* iAction, void* function = 0);


    bool disconnect(SContextIO* iAction, void* function = 0);


    bool receive(SContextIO* iAction);


    bool send(SContextIO* iAction);


    void* getFunctionDisconnect()const;


    void* getFunctionConnect()const;


    void* getFunctionAccpet()const;


    void* getFunctionAcceptSockAddress()const;

#endif//APP_PLATFORM_WINDOWS

protected:
    netsocket mSocket;
};




#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
class CNetSocketPair {
public:
    CNetSocketPair();

    ~CNetSocketPair();

    bool close();

    /**
    * @brief Open a local socket pair.
    * @return true if success to open, else false.
    */
    bool open();

    bool open(s32 domain, s32 type, s32 protocol);

    CNetSocket& getSocketA() {
        return mSockA;
    }

    CNetSocket& getSocketB() {
        return mSockB;
    }

private:
    CNetSocket mSockA;
    CNetSocket mSockB;
};
#endif //OS APP_PLATFORM_LINUX  APP_PLATFORM_ANDROID


} //namespace net 
} //namespace irr 


#endif //APP_CNETSOCKET_H