#ifndef APP_CNETADDRESS_H
#define APP_CNETADDRESS_H

#include "HNetConfig.h"
#include "irrString.h"

#if defined(APP_NET_USE_IPV6)
#include "IBigInteger.h"
struct sockaddr_in6;

//ipv6 IP is no more than 39 chars, it's like "CDCD:910A:2222:5498:8475:1111:3900:2020"
#define APP_IP_STRING_MAX_SIZE 40

#else

struct sockaddr_in;

//ipv4 IP is no more than 16 chars, it's like "127.122.122.122"
#define APP_IP_STRING_MAX_SIZE 16
#endif


namespace irr {
namespace net {

/**
* @brief CNetAddress is a usefull struct to deal with IP & port.
* @note IP and Port are in network-byte-order(big endian).
*/
class CNetAddress {
public:
#if defined(APP_NET_USE_IPV6)
    typedef IBigInteger<u8[18]> ID; //big endian as network
    typedef IBigInteger<u8[16]> IP; //big endian as network
#else
    typedef u64 ID; //big endian as network
    typedef u32 IP; //big endian as network
#endif

    /**
    *@brief Construct a net address with any ip and any port.
    *eg: "0.0.0.0:0"
    */
    CNetAddress();

    /**
    *@brief Construct a net address with "user defined ip" and any port.
    *@param ip User defined ip.
    */
    CNetAddress(const c8* ip);

    /**
    *@brief Construct a net address with "user defined ip" and any port.
    *@param ip User defined ip.
    */
    CNetAddress(const core::stringc& ip);

    /**
    *@brief Construct a net address with "user defined port" and any ip.
    *@param port User defined port.
    */
    CNetAddress(u16 port);

    ~CNetAddress();

    CNetAddress(const c8* ip, u16 port);

    CNetAddress(const core::stringc& ip, u16 port);

    CNetAddress(const CNetAddress& other);

    CNetAddress& operator=(const CNetAddress& other);

    bool operator==(const CNetAddress& other) const;

    bool operator!=(const CNetAddress& other) const;

    /**
    *@brief Get family of this net address.
    *@return Family, in big endian.
    */
    u16 getFamily()const;

    /**
    *@brief Set IP, the real address will auto inited.
    *@param ip User defined IP.
    */
    void setIP(const c8* ip);

    /**
    *@brief Set IP.
    *@param ip User defined IP.
    */
    void setIP(const core::stringc& ip) {
        setIP(ip.c_str());
    }

    /**
    *@brief Use local IP.
    *@return True if success to got local IP, else false.
    */
    bool setIP();

    /**
    *@brief Set IP.
    *@param ip IP in big endian.
    */
    void setIP(const IP& id);

    /**
    *@brief Set port, the real address will auto inited.
    *@param port User defined port, in OS endian.
    */
    void setPort(u16 port);

    /**
    *@brief Get port.
    *@return The port, in OS endian.
    */
    u16 getPort()const;

    /**
    *@brief Get port.
    *@return The port, in big endian.
    */
    u16 getNetPort()const {
        return APP_SWAP16(getPort());
    }

    /**
    *@brief Set IP by a DNS, the real address will auto inited.
    *@param iDNS User defined domain, eg: "www.google.com".
    */
    void setDomain(const c8* iDNS);

    /**
    *@brief Set IP:Port, the real address will auto inited.
    *@param ip User defined ip.
    *@param port User defined port, in OS-endian.
    */
    void set(const c8* ip, u16 port);

    /**
    *@brief Set IP and Port
    *@param ipAndPort format is "127.0.0.1:8080"
    */
    void setIPort(const c8* ipAndPort);

    /**
    *@brief Set IP:Port, the real address will auto inited.
    *@param ip User defined ip.
    *@param port User defined port, in OS-endian.
    */
    void set(const core::stringc& ip, u16 port) {
        set(ip.c_str(), port);
    }

    /**
    * @return ID of this address.
    */
    const ID& getID()const {
        return mID;
    }

    /**
    * @return IP, in big endian.
    */
    const IP& getIP() const;

    const char* getIPString() const {
        return mIP;
    }

#if defined(APP_NET_USE_IPV6)
    void setAddress(const sockaddr_in6& it);

    APP_FORCE_INLINE const sockaddr_in6* getAddress() const {
        return reinterpret_cast<const sockaddr_in6*>(mAddress);
    }
    APP_FORCE_INLINE sockaddr_in6* getAddress() {
        return reinterpret_cast<sockaddr_in6*>(mAddress);
    }
#else
    void setAddress(const sockaddr_in& it);

    APP_FORCE_INLINE const sockaddr_in* getAddress() const {
        return reinterpret_cast<const sockaddr_in*>(mAddress);
    }
    APP_FORCE_INLINE sockaddr_in* getAddress() {
        return reinterpret_cast<sockaddr_in*>(mAddress);
    }
#endif

    void reverse();

    /**
    *@param ip The IP string to convert.
    *@param result The converted ip, in big endian.
    */
    static void convertStringToIP(const c8* ip, IP& result);

    /**
    *@param ip The IP to convert, in big endian.
    *@param result The converted ip string.
    */
    static void convertIPToString(const IP& ip, core::stringc& result);

private:
    ///init when created
    APP_INLINE void init();
    /**
    *@brief Auto inited again if IP or Port changed.
    */
    APP_INLINE void initIP();
    APP_INLINE void initPort();
    APP_INLINE void mergeIP();
    APP_INLINE void mergePort();

    char mIP[APP_IP_STRING_MAX_SIZE];
    u16 mPort;  ///<OS endian
    ID mID;     ///<merged IP&Port, in big endian as network

#if defined(APP_NET_USE_IPV6)
    //sizeof(sockaddr_in6)=28
    s8 mAddress[28];
#else
    //sizeof(sockaddr_in)=16
    s8 mAddress[16];
#endif

};


}//namespace net
}//namespace irr

#endif //APP_CNETADDRESS_H