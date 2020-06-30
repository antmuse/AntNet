#include "CNetPing.h"
#include "CNetUtility.h"
#include "CCheckSum.h"
#include "CTimer.h"
//#include "IUtility.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#define APP_PACKET_ID ::GetCurrentProcessId()
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
#define APP_PACKET_ID 0xCC
#endif  //APP_PLATFORM_WINDOWS



namespace app {
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


bool CNetPing::ping(const s8* remoteIP, u32 max, s32 timeout) {
    if(!mSocket.openRaw(IPPROTO_ICMP)) {
        return false;
    }

    /*if(mSocket.setReceiveOvertime(5000)) {
        return false;
    }*/
    if(mSocket.setSendOvertime(timeout)) {
        return false;
    }

    mAddressRemote.setIP(remoteIP);

    // �����������ݰ����ȷ����ڴ棬�ڵ���FillCMPData���SHeadICMP�ṹ      
    mDataSize += sizeof(SHeadICMP);
    mICMP_Data = (s8*) ::malloc(MAX_PACKET);
    mReceiveBuffer = (s8*) ::malloc(MAX_PACKET);
    if(!mICMP_Data) {
        return false;
    }
    ::memset(mICMP_Data, 0, MAX_PACKET);
    writeICMPData(mICMP_Data, mDataSize);

    CCheckSum cksum;
    for(u32 nCount = 0; nCount < max; ++nCount) {
        cksum.clear();
        ((SHeadICMP*) mICMP_Data)->mChecksum = 0;
        ((SHeadICMP*) mICMP_Data)->mTimestamp = CTimer::getRelativeTime();;
        ((SHeadICMP*) mICMP_Data)->mSN = mSendSN++;
        cksum.add(mICMP_Data, mDataSize);
        ((SHeadICMP*) mICMP_Data)->mChecksum = cksum.get();
        if(mSocket.sendto(mICMP_Data, mDataSize, mAddressRemote) < 0) {
            if(ESEC_TIMEOUT == mSocket.getError()) {
                // ("timed out--Send");
                continue;
            }
            //("sendto() failed");
            return false;
        }

        s32 bread = mSocket.receiveFrom(mReceiveBuffer, MAX_PACKET, mAddressFrom);
        if(bread < 0) {
            if(ESEC_TIMEOUT == mSocket.getError()) {
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


void CNetPing::writeICMPData(s8* mICMP_Data, s32 datasize) {
    SHeadICMP* icmp_hdr = (SHeadICMP*) mICMP_Data;
    icmp_hdr->mType = ICMP_ECHO;        // Request an ICMP echo
    icmp_hdr->mCode = 0;
    icmp_hdr->mID = (u16) APP_PACKET_ID;
    icmp_hdr->mChecksum = 0;
    icmp_hdr->mSN = 0;

    s8* datapart = mICMP_Data + sizeof(SHeadICMP);
}


bool CNetPing::decodeHeader(s8 *buf, s32 bytes, CNetAddress& from) {
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
    if(icmphdr->mID != (u16) APP_PACKET_ID) {
        //("someone else's packet!\n");
        return false;
    }
    return true;
}

}//namespace net
}//namespace app
