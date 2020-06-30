// Ping 类：实现对目标主机的Ping 探测

#ifndef APP_CNETPING_H
#define APP_CNETPING_H

#include "HNetHeader.h"
#include "CNetAddress.h"
#include "CNetSocket.h"

namespace app {
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

    void writeICMPData(s8* mICMP_Data, s32 datasize);

    bool decodeHeader(s8* buf, s32 bytes, CNetAddress& from);

    void clear();

    /**
    * @brief Ping a remote IP.
    * @param remoteIP Remote IP.
    * @param max Max ping count.
    * @param timeout Timeout.
    * @return True if success, else false.
    */
    bool ping(const s8* remoteIP, u32 max, s32 timeout = 1000);

protected:
    CNetSocket mSocket;
    SHeadOptionIP mOption;
    CNetAddress mAddressRemote;
    CNetAddress mAddressFrom;
    s8* mICMP_Data;
    s8* mReceiveBuffer;
    u16 mSendSN;
    s32 mDataSize;
};

}//namespace net
}//namespace app

#endif //APP_CNETPING_H
