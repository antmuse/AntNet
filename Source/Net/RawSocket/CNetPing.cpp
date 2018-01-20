#include "CNetPing.h"
#include "CNetUtility.h"
#include "CCheckSum.h"
//#include "IUtility.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
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
#endif  //APP_PLATFORM_WINDOWS


namespace irr {
namespace net {

CNetPing::CNetPing() :
    mDataSize(DEF_PACKET_SIZE),
    mSendSN(0),
    mICMP_Data(0),
    mReceiveBuffer(0) {
    CNetUtility::loadSocketLib();
}


CNetPing::~CNetPing() {
    CNetUtility::unloadSocketLib();
    clear();
}


bool CNetPing::ping(const c8* remoteIP, u32 max, s32 timeout) {
    if(!mSocket.open(AF_INET, SOCK_RAW, IPPROTO_ICMP)) {
        return false;
    }

    /*if(mSocket.setReceiveOvertime(5000)) {
        return false;
    }*/
    if(mSocket.setSendOvertime(timeout)) {
        return false;
    }

    mAddressRemote.setIP(remoteIP);

    // 创建报文数据包，先分配内存，在调用FillCMPData填充SHeadICMP结构      
    mDataSize += sizeof(SHeadICMP);
    mICMP_Data = (c8*) ::malloc(MAX_PACKET);
    mReceiveBuffer = (c8*) ::malloc(MAX_PACKET);
    if(!mICMP_Data) {
        return false;
    }
    ::memset(mICMP_Data, 0, MAX_PACKET);
    writeICMPData(mICMP_Data, mDataSize);

    CCheckSum cksum;
    for(s32 nCount = 0; nCount < max; ++nCount) {
        cksum.clear();
        ((SHeadICMP*) mICMP_Data)->mChecksum = 0;
        ((SHeadICMP*) mICMP_Data)->mTimestamp = ::GetTickCount();
        ((SHeadICMP*) mICMP_Data)->mSN = mSendSN++;
        cksum.add(mICMP_Data, mDataSize);
        ((SHeadICMP*) mICMP_Data)->mChecksum = cksum.get();
        if(mSocket.sendto(mICMP_Data, mDataSize, mAddressRemote) < 0) {
            if(mSocket.getError() == WSAETIMEDOUT) {
                // ("timed out--Send");
                continue;
            }
            //("sendto() failed");
            return false;
        }

        s32 bread = mSocket.receiveFrom(mReceiveBuffer, MAX_PACKET, mAddressFrom);
        if(bread == SOCKET_ERROR) {
            if(mSocket.getError() == WSAETIMEDOUT) {
                //("timed out");
                return false;
            }
            //("recvfrom() failed");
            return false;
        }
        if(!decodeHeader(mReceiveBuffer, bread, mAddressFrom)) {
            return false;
        }
    }//for

    mSocket.close();
    return true;
}


void CNetPing::clear() {
    mSocket.close();
    ::free(mReceiveBuffer);
    ::free(mICMP_Data);
    mReceiveBuffer = 0;
    mICMP_Data = 0;
}


void CNetPing::writeICMPData(c8* mICMP_Data, s32 datasize) {
    SHeadICMP* icmp_hdr = (SHeadICMP*) mICMP_Data;
    icmp_hdr->mType = ICMP_ECHO;        // Request an ICMP echo
    icmp_hdr->mCode = 0;
    icmp_hdr->mID = (u16) ::GetCurrentProcessId();
    icmp_hdr->mChecksum = 0;
    icmp_hdr->mSN = 0;

    c8* datapart = mICMP_Data + sizeof(SHeadICMP);
}


bool CNetPing::decodeHeader(c8 *buf, s32 bytes, SNetAddress& from) {
    SHeadIP* iphdr = (SHeadIP*) buf;
    u16 iphdrlen = iphdr->getSize();
    if(bytes < iphdrlen + ICMP_MIN) {
        //("Too few bytes");
        return false;
    }
    SHeadICMP* icmphdr = (SHeadICMP*) (buf + iphdrlen);

    if(icmphdr->mType != ICMP_ECHOREPLY) {
        //("nonecho type");
        return false;
    }
    // Make sure this is an ICMP reply to something we sent!
    if(icmphdr->mID != (u16) ::GetCurrentProcessId()) {
        //("someone else's packet!\n");
        return false;
    }
    return true;
}

}//namespace net
}//namespace irr