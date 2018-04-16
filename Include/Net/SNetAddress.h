#ifndef APP_SNETADDRESS_H
#define APP_SNETADDRESS_H

#include "HNetConfig.h"
#include "irrString.h"

#if defined(APP_NET_USE_IPV6)
#include "IBigInteger.h"
struct sockaddr_in6;
#else
struct sockaddr_in;
#endif


namespace irr {
namespace net {

/**
* @brief SNetAddress is a usefull struct to deal with IP & port.
* @note IP and Port are in network-byte-order(big endian).
*/
struct SNetAddress {
public:
#if defined(APP_NET_USE_IPV6)
    sockaddr_in6* mAddress;
    typedef IBigInteger<u8[18]> ID; //big endian as network
    typedef IBigInteger<u8[16]> IP; //big endian as network
#else
    sockaddr_in* mAddress;
    typedef u64 ID; //big endian as network
    typedef u32 IP; //big endian as network
#endif
    core::stringc mIP;
    u16 mPort;  ///<OS endian
    ID mID;     ///<merged IP&Port, in big endian as network
         
    /**
    *@brief Construct a net address with any ip and any port.
    *eg: "0.0.0.0:0"
    */
    SNetAddress();

    /**
    *@brief Construct a net address with "user defined ip" and any port.
    *@param ip User defined ip.
    */
    SNetAddress(const c8* ip);

    /**
    *@brief Construct a net address with "user defined ip" and any port.
    *@param ip User defined ip.
    */
    SNetAddress(const core::stringc& ip);

    /**
    *@brief Construct a net address with "user defined port" and any ip.
    *@param port User defined port.
    */
    SNetAddress(u16 port);

    ~SNetAddress();

    SNetAddress(const c8* ip, u16 port);

    SNetAddress(const core::stringc& ip, u16 port);

    SNetAddress(const SNetAddress& other);

    SNetAddress& operator=(const SNetAddress& other);

    bool operator==(const SNetAddress& other) const;

    bool operator!=(const SNetAddress& other) const;

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
    *@return The port, in big endian.
    */
    u16 getPort()const;

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
    *@brief Set IP:Port, the real address will auto inited.
    *@param ip User defined ip.
    *@param port User defined port, in OS-endian.
    */
    void set(const core::stringc& ip, u16 port) {
        set(ip.c_str(), port);
    }

    /**
    *@brief Get IP:Port from a valid real address[sockaddr_in]
    */
    void reverse();

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


#if defined(APP_NET_USE_IPV6)
    //sizeof(sockaddr_in6)=28
    s8 mCache[28];
#else
    //sizeof(sockaddr_in)=16
    s8 mCache[16];
#endif

};


}//namespace net
}//namespace irr

#endif //APP_SNETADDRESS_H