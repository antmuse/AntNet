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

struct SNetAddress {
public:
#if defined(APP_NET_USE_IPV6)
    sockaddr_in6* mAddress;
    typedef IBigInteger<u8[18]> ID;
    typedef IBigInteger<u8[16]> IP;
#else
    sockaddr_in* mAddress;
    typedef u64 ID;
    typedef u32 IP;
#endif
    core::stringc mIP;
    u16 mPort;
    ID mID;

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


    u16 getFamily()const;

    /**
    *@brief Set IP, the real address will auto inited.
    *@param ip User defined ip.
    */
    void setIP(const c8* ip);

    /**
    *@brief Set port, the real address will auto inited.
    *@param port User defined port.
    */
    void setPort(u16 port);

    /**
    *@brief Set IP by a DNS, the real address will auto inited.
    *@param iDNS User defined domain, eg: "www.google.com".
    */
    void setDomain(const c8* iDNS);

    /**
    *@brief Set IP:Port, the real address will auto inited.
    *@param ip User defined ip.
    *@param port User defined port.
    */
    void set(const c8* ip, u16 port);

    /**
    *@brief Get IP:Port from a valid real address[sockaddr_in]
    */
    void reverse();

    const ID& getID()const {
        return mID;
    }

    const IP& getIPAsID() const;

    void setIP(u32 id) {
        setIP(SNetAddress::getIDAsIP(id).c_str());
    }

    /**
    *@param ip The ip string to convert.
    *@return The ID of the ip string, in big endian.
    */
    static IP getIPAsID(const c8* ip);

    /**
    *@param ip The id of IP, in big endian.
    *@return The ip string of id.
    */
    static core::stringc getIDAsIP(IP& ip);

private:
    /**
    *@brief Auto inited again if IP or Port changed.
    */
    APP_INLINE void initIP();
    APP_INLINE void initPort();
    APP_INLINE void mergeIP();
    APP_INLINE void mergePort();
};


}//namespace net
}//namespace irr

#endif //APP_SNETADDRESS_H