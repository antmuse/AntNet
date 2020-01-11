// Ping 类：实现对目标主机的Ping 探测

#ifndef APP_CNETPING_H
#define APP_CNETPING_H

#include "HNetHeader.h"
#include "CNetAddress.h"
#include "CNetSocket.h"

namespace irr {
namespace net {

#define DEF_PACKET_SIZE  32 
#define MAX_PACKET       1024      //ICMP报的最大长度 Max ICMP packet size
#define MAX_IP_HDR_SIZE  60       
#define ICMP_ECHO        8
#define ICMP_ECHOREPLY   0
#define ICMP_MIN         8          // ICMP报的最小长度Minimum 8-byte ICMP packet (header)

class CNetPing {
public:
    CNetPing();

    virtual ~CNetPing();

    void writeICMPData(c8* mICMP_Data, s32 datasize);

    bool decodeHeader(c8* buf, s32 bytes, CNetAddress& from);

    void clear();

    /**
    * @brief Ping a remote IP.
    * @param remoteIP Remote IP.
    * @param max Max ping count.
    * @param timeout Timeout.
    * @return True if success, else false.
    */
    bool ping(const c8* remoteIP, u32 max, s32 timeout = 1000);

protected:
    CNetSocket mSocket;
    SHeadOptionIP mOption;
    CNetAddress mAddressRemote;
    CNetAddress mAddressFrom;
    c8* mICMP_Data;
    c8* mReceiveBuffer;
    u16 mSendSN;
    s32 mDataSize;
};

}//namespace net
}//namespace irr

#endif //APP_CNETPING_H
