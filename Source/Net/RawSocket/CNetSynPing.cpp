#include "CNetSynPing.h"
#include "CNetUtility.h"
#include "IAppLogger.h"
#include "IAppTimer.h"
#include "CCheckSum.h"
#include "IUtility.h"
#include <string>

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


#define APP_SN_START 0x9917bf05


namespace irr {
namespace net {

CNetSynPing::CNetSynPing() {
    CNetUtility::loadSocketLib();
}


CNetSynPing::~CNetSynPing() {
    CNetUtility::unloadSocketLib();
}


void CNetSynPing::closeAll() {
    mScoketRaw.close();
    mScoketListener.close();
}


bool CNetSynPing::init() {
    APP_ASSERT(20 == sizeof(SHeadIP));
    APP_ASSERT(20 == sizeof(SHeadTCP));
    APP_ASSERT(sizeof(SHeadIP) > sizeof(SFakeHeadTCP));
    APP_ASSERT(APP_SWAP32(0xC1) == ::htonl(0xC1));
    IAppLogger::log(ELOG_INFO, "CNetSynPing::init", "sizeof SHeadIP=%u", sizeof(SHeadIP));
    IAppLogger::log(ELOG_INFO, "CNetSynPing::init", "sizeof SHeadTCP=%u", sizeof(SHeadTCP));
    IAppLogger::log(ELOG_INFO, "CNetSynPing::init", "sizeof SFakeHeadTCP=%u", sizeof(SFakeHeadTCP));
    IAppLogger::log(ELOG_INFO, "CNetSynPing::init", "sizeof SHeadOptionTCP=%u", sizeof(SHeadOptionTCP));
    //�ػ�ip���ݰ���   socket(AF_INET, SOCK_RAW, IPPROTO_TCP|IPPROTO_UDP|IPPROTO_ICMP)
    //�ػ���̫������֡��socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP | ETH_P_ARP | ETH_P_ALL))
    if(!mScoketRaw.open(AF_INET, SOCK_RAW, IPPROTO_IP)) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::init", "mScoketRaw.open,%u", CNetSocket::getError());
        return false;
    }
    if(mScoketRaw.setCustomIPHead(true)) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::init", "setCustomIPHead,%u", CNetSocket::getError());
        closeAll();
        return false;
    }
    if(!mScoketListener.open(AF_INET, SOCK_RAW, IPPROTO_IP)) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::init", "mScoketListener.open,%u", CNetSocket::getError());
        closeAll();
        return false;
    }
    if(!mAddressLocal.setIP()) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::init", "get mAddressLocal IP,%u", CNetSocket::getError());
        closeAll();
        return false;
    }

    mAddressLocal.setPort(54436);

    if(mScoketListener.bind(mAddressLocal)) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::init", "bind error,%u", CNetSocket::getError());
        closeAll();
        return false;
    }


    u32 dwBufferLen[10];
    u32 dwBufferInLen = RCVALL_ON;
    DWORD dwBytesReturned = 0;
    //::ioctlsocket(mScoketListener.getValue(), SIO_RCVALL, &dwBufferInLen);
    if(::WSAIoctl(mScoketListener.getValue(), SIO_RCVALL,
        &dwBufferInLen, sizeof(dwBufferInLen),
        &dwBufferLen, sizeof(dwBufferLen),
        &dwBytesReturned, 0, 0)) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::init", "WSAIoctl, %u", CNetSocket::getError());
        closeAll();
        return false;
    }

    return true;
}


bool CNetSynPing::send() {
    CCheckSum sumchecker;
    c8 sendCache[sizeof(SHeadIP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP)];
    SHeadIP&  ipHead = *(SHeadIP*) sendCache;
    SHeadTCP& tcpHead = *(SHeadTCP*) (sendCache + sizeof(SHeadIP));
    SFakeHeadTCP& fakeHeadTCP = *(SFakeHeadTCP*) (sendCache + sizeof(SHeadIP) - sizeof(SFakeHeadTCP));
    SHeadOptionTCP& optionTCP = *(SHeadOptionTCP*) (sendCache + sizeof(SHeadIP) + sizeof(SHeadTCP));

    /*c8 buf[40];
    ::inet_ntop(mAddressRemote.mAddress->sin_family, &mAddressRemote.getIP(), buf, sizeof(buf));
    IAppLogger::log(ELOG_INFO, "CNetSynPing::send", "%s == %s", buf, mAddressRemote.mIP.c_str());
    IAppLogger::log(ELOG_INFO, "CNetSynPing::send", "mAddressLocal  IP= 0x%x:0x%x",
    mAddressLocal.getIP(), mAddressLocal.getPort());
    IAppLogger::log(ELOG_INFO, "CNetSynPing::send", "mAddressRemote IP= 0x%x:0x%x",
    mAddressRemote.getIP(), mAddressRemote.getPort());*/

    //���TCPα�ײ�
    fakeHeadTCP.mLocalIP = mAddressLocal.getIP();
    fakeHeadTCP.mRemoteIP = mAddressRemote.getIP();
    fakeHeadTCP.mPadding = 0;
    fakeHeadTCP.mProtocol = IPPROTO_TCP;
    fakeHeadTCP.setSize(sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));
    //���TCP�ײ�
    tcpHead.mLocalPort = mAddressLocal.getPort();
    tcpHead.mRemotePort = mAddressRemote.getPort();
    tcpHead.setSN(APP_SN_START);
    tcpHead.mACK = 0;
    tcpHead.mSizeReserveFlag = 0;                                    //step 1
    tcpHead.setSize(sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));      //step 2
    tcpHead.setFlag(SHeadTCP::EFLAG_SYN);                            //step 3
    tcpHead.setWindowSize(64240);
    tcpHead.mOffset = 0;
    tcpHead.mChecksum = 0;
    optionTCP.mType = SHeadOptionTCP::ETYPE_MSS;
    optionTCP.mSize = 4;
    optionTCP.setMSS(1460);
    optionTCP.mOther[0] = SHeadOptionTCP::ETYPE_NO_OPTION;
    optionTCP.mOther[1] = SHeadOptionTCP::ETYPE_WIN_SCALE;
    optionTCP.mOther[2] = 3;
    optionTCP.mOther[3] = 8;
    optionTCP.mOther[4] = SHeadOptionTCP::ETYPE_NO_OPTION;
    optionTCP.mOther[5] = SHeadOptionTCP::ETYPE_NO_OPTION;
    optionTCP.mOther[6] = SHeadOptionTCP::ETYPE_SACK_PERMITTED;
    optionTCP.mOther[7] = 2;
    //TCPУ���=fack head + tcp head         
    sumchecker.clear();
    sumchecker.add((c8*) &fakeHeadTCP, sizeof(SFakeHeadTCP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));
    tcpHead.mChecksum = sumchecker.get();


    //���IP�ײ�
    ipHead.setVersion(4);
    ipHead.setSize(sizeof(SHeadIP));
    ipHead.mType = 0;
    ipHead.setTotalSize(sizeof(SHeadIP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));
    ipHead.setIdent(14480);
    ipHead.mFlag = 0;                       //step1
    ipHead.setFlag(SHeadIP::EFLAG_NO_FRAG); //step2
    ipHead.mTTL = 128;
    ipHead.mProtocol = IPPROTO_TCP;
    ipHead.mChecksum = 0;
    ipHead.mLocalIP = mAddressLocal.getIP();
    ipHead.mRemoteIP = mAddressRemote.getIP();
    //IPУ���=ip head
    sumchecker.clear();
    sumchecker.add((c8*) &ipHead, sizeof(SHeadIP));
    ipHead.mChecksum = sumchecker.get();

    printf("\n----------------------------------------------------------------\n");
    printf("IP = ");
    IUtility::print((u8*) &ipHead, sizeof(ipHead));
    printf("\n");
    printf("TCP = ");
    IUtility::print((u8*) &tcpHead, sizeof(tcpHead));
    printf("\n");
    printf("TCP option = ");
    IUtility::print((u8*) &optionTCP, sizeof(optionTCP));
    printf("\n----------------------------------------------------------------\n");

    //send out
    s32 datasize = sizeof(SHeadIP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP);
    datasize = mScoketRaw.sendto(sendCache, datasize, mAddressRemote);
    if(datasize < 0) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::send", "send,%u", CNetSocket::getError());
        return false;
    }
    IAppLogger::log(ELOG_INFO, "CNetSynPing::send", "send OK, %u", datasize);
    return true;
}


bool CNetSynPing::sendReset() {
    CCheckSum sumchecker;
    c8 sendCache[sizeof(SHeadIP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP)];
    SHeadIP&  ipHead = *(SHeadIP*) sendCache;
    SHeadTCP& tcpHead = *(SHeadTCP*) (sendCache + sizeof(SHeadIP));
    SFakeHeadTCP& fakeHeadTCP = *(SFakeHeadTCP*) (sendCache + sizeof(SHeadIP) - sizeof(SFakeHeadTCP));
    SHeadOptionTCP& optionTCP = *(SHeadOptionTCP*) (sendCache + sizeof(SHeadIP) + sizeof(SHeadTCP));

    //���TCPα�ײ�
    fakeHeadTCP.mLocalIP = mAddressLocal.getIP();
    fakeHeadTCP.mRemoteIP = mAddressRemote.getIP();
    fakeHeadTCP.mPadding = 0;
    fakeHeadTCP.mProtocol = IPPROTO_TCP;
    fakeHeadTCP.setSize(sizeof(SFakeHeadTCP) + sizeof(SHeadTCP));
    //���TCP�ײ�
    tcpHead.mLocalPort = mAddressLocal.getPort();
    tcpHead.mRemotePort = mAddressRemote.getPort();
    tcpHead.setSN(APP_SN_START);
    tcpHead.mACK = 0;
    tcpHead.mSizeReserveFlag = 0;                                    //step 1
    tcpHead.setSize(sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));      //step 2
    tcpHead.setFlag(SHeadTCP::EFLAG_RST | SHeadTCP::EFLAG_ACK);      //step 3
    tcpHead.setWindowSize(64240);
    tcpHead.mOffset = 0;
    tcpHead.mChecksum = 0;
    optionTCP.mType = SHeadOptionTCP::ETYPE_MSS;
    optionTCP.mSize = 4;
    optionTCP.setMSS(1460);
    optionTCP.mOther[0] = SHeadOptionTCP::ETYPE_NO_OPTION;
    optionTCP.mOther[1] = SHeadOptionTCP::ETYPE_WIN_SCALE;
    optionTCP.mOther[2] = 3;
    optionTCP.mOther[3] = 8;
    optionTCP.mOther[4] = SHeadOptionTCP::ETYPE_NO_OPTION;
    optionTCP.mOther[5] = SHeadOptionTCP::ETYPE_NO_OPTION;
    optionTCP.mOther[6] = SHeadOptionTCP::ETYPE_SACK_PERMITTED;
    optionTCP.mOther[7] = 2;
    //TCPУ���=fack head + tcp head         
    sumchecker.clear();
    sumchecker.add((c8*) &fakeHeadTCP, sizeof(SFakeHeadTCP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));
    tcpHead.mChecksum = sumchecker.get();

    //���IP�ײ�
    ipHead.setVersion(4);
    ipHead.setSize(sizeof(SHeadIP));
    ipHead.mType = 0;
    ipHead.setTotalSize(sizeof(SHeadIP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP));
    ipHead.setIdent(7003);
    ipHead.mFlag = 0;                       //step1
    ipHead.setFlag(SHeadIP::EFLAG_NO_FRAG); //step2
    ipHead.mTTL = 128;
    ipHead.mProtocol = IPPROTO_TCP;
    ipHead.mChecksum = 0;
    ipHead.mLocalIP = mAddressLocal.getIP();
    ipHead.mRemoteIP = mAddressRemote.getIP();
    //IPУ���=ip head
    sumchecker.clear();
    sumchecker.add((c8*) &ipHead, sizeof(SHeadIP));
    ipHead.mChecksum = sumchecker.get();

    printf("IP = ");
    IUtility::print((u8*) &ipHead, sizeof(ipHead));
    printf("\n--------------------------------------\n");
    printf("TCP = ");
    IUtility::print((u8*) &tcpHead, sizeof(tcpHead));
    printf("\n--------------------------------------\n");
    printf("TCP option = ");
    IUtility::print((u8*) &optionTCP, sizeof(optionTCP));
    printf("\n--------------------------------------\n");


    //send out
    s32 datasize = sizeof(SHeadIP) + sizeof(SHeadTCP) + sizeof(SHeadOptionTCP);
    datasize = mScoketRaw.sendto(sendCache, datasize, mAddressRemote);
    if(datasize < 0) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::sendReset", "send,%u", CNetSocket::getError());
        return false;
    }
    IAppLogger::log(ELOG_INFO, "CNetSynPing::sendReset", "send OK, %u", datasize);
    return true;
}


s32 CNetSynPing::ping(const c8* remoteIP, u16 remotePort) {
    mAddressRemote.set(remoteIP, remotePort);
    IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "%s:%u --> %s:%u",
        mAddressLocal.mIP.c_str(), mAddressLocal.mPort, mAddressRemote.mIP.c_str(), mAddressRemote.mPort);
    if(!send()) {
        IAppLogger::log(ELOG_ERROR, "CNetSynPing::ping", "send fail, %u", CNetSocket::getError());
        closeAll();
        return -1;
    }

    SFakeHeadTCP fakeTCP;
    CCheckSum sumchecker;
    c8 recvCache[65535] = {0};
    s32 ecode;
    u32 timeout = 2000;
    u64 start = IAppTimer::getTime();
    core::stringc localips, remoteips;

    while(true) {
        if((IAppTimer::getTime() - start) >= timeout) {
            IAppLogger::log(ELOG_ERROR, "CNetSynPing::ping", "receive timeOut, %u", CNetSocket::getError());
            //if(!send()) break;
            break;
        }
        ::memset(recvCache, 0, sizeof(recvCache));
        ecode = mScoketListener.receive(recvCache, sizeof(recvCache));
        if(ecode < 0) {
            IAppLogger::log(ELOG_ERROR, "CNetSynPing::ping", "receive fail, %u", CNetSocket::getError());
            break;
        }

        SHeadIP& recvHeadIP = *(SHeadIP*) recvCache;
        if(IPPROTO_TCP != recvHeadIP.mProtocol) {
            continue;
        }
        u8 recvIPsize = recvHeadIP.getSize();
        u16 totalSize = recvHeadIP.getTotalSize();
        SHeadTCP& recvHeadTCP = *(SHeadTCP*) (recvCache + recvIPsize);
        SNetAddress::convertIPToString(recvHeadIP.mLocalIP, localips);
        SNetAddress::convertIPToString(recvHeadIP.mRemoteIP, remoteips);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "--------------------------------------recived, %u", ecode);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP Version = %u]", recvHeadIP.getVersion());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP Ident = %u]", recvHeadIP.getIdent());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP Flag = 0x%x]", recvHeadIP.getFlag());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP TTL = %u]", recvHeadIP.mTTL);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP Protocol = %u]", recvHeadIP.mProtocol);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP Size = %u/%u]", recvIPsize, totalSize);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [IP Checksum = 0x%x]", recvHeadIP.mChecksum);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP SN = %u]", recvHeadTCP.getSN());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP IP:Port = %s:%u --> %s:%u]",
            localips.c_str(), recvHeadTCP.getLocalPort(), remoteips.c_str(), recvHeadTCP.getRemotePort());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP Checksum = 0x%x]", recvHeadTCP.mChecksum);
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP ACK = %u]", recvHeadTCP.getACK());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP Size = %u]", recvHeadTCP.getSize());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP Flag = 0x%x]", recvHeadTCP.getFlag());
        IAppLogger::log(ELOG_INFO, "CNetSynPing::ping", "receive [TCP Window = %u]", recvHeadTCP.getWindowSize());


        u16 cksumbk = recvHeadIP.mChecksum;
        recvHeadIP.mChecksum = 0;
        sumchecker.clear();
        sumchecker.add(recvCache, recvIPsize);
        if(sumchecker.get() != cksumbk) {
            IAppLogger::log(ELOG_ERROR, "CNetSynPing::ping", "IP checksum fail, 0x%x", cksumbk);
            continue;
        }
        fakeTCP.mLocalIP = recvHeadIP.mLocalIP;
        fakeTCP.mRemoteIP = recvHeadIP.mRemoteIP;
        fakeTCP.mPadding = 0;
        fakeTCP.mProtocol = recvHeadIP.mProtocol;
        fakeTCP.setSize(recvHeadTCP.getSize());
        sumchecker.clear();
        sumchecker.add(&fakeTCP, sizeof(fakeTCP));
        cksumbk = recvHeadTCP.mChecksum;
        recvHeadTCP.mChecksum = 0;
        sumchecker.add(&recvHeadTCP, totalSize - recvIPsize);
        if(sumchecker.get() != cksumbk) {
            IAppLogger::log(ELOG_ERROR, "CNetSynPing::ping", "TCP checksum fail, 0x%x", cksumbk);
            continue;
        }
        if(recvHeadIP.mLocalIP != mAddressRemote.getIP()) {
            continue;
        }
        if(recvHeadIP.mRemoteIP != mAddressLocal.getIP()) {
            continue;
        }
        if(recvHeadTCP.mLocalPort != mAddressRemote.getPort()) {
            continue;
        }
        if(recvHeadTCP.mRemotePort != mAddressLocal.getPort()) {
            continue;
        }

        if(recvHeadTCP.getACK() != (APP_SN_START + 1)
            && recvHeadTCP.getACK() != APP_SN_START) {
            continue;
        }

        //RST/ACK - �޷���  the port is not open.
        if(recvHeadTCP.getFlag() == 20) {            //("�������ڵ�����Ӧ");
            closeAll();
            return 1;
        }

        //SYN/ACK - ɨ�赽һ���˿�.the port is open
        if(recvHeadTCP.getFlag() == 18) { //("�˿ڿ���!");
            if(!sendReset()) {
                //("Send Error!!:%d\n", GetLastError());
            }
            closeAll();
            return 2;
        }
    }//while

    closeAll();
    return 0;
}



}//namespace net
}//namespace irr