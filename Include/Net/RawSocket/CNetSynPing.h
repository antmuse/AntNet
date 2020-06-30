// SYNPing�ࣺʵ�ֶ�Ŀ������ָ���˿ڵ�SYN̽���ɨ��

#ifndef APP_CNETSYNPING_H
#define APP_CNETSYNPING_H

#include "HNetHeader.h"
#include "CNetSocket.h"


namespace app {
namespace net {



class CNetSynPing {
public:
    CNetSynPing();
    virtual ~CNetSynPing();

    bool init();

    //ʵ��Ping����
    //0: ����������
    //1: �������ڵ�û����ָ���˿�
    //2: �������ڲ�����ָ���˿�
    s32 ping(const s8* remoteIP, u16 remotePort);

protected:
    CNetAddress mAddressLocal;
    CNetAddress mAddressRemote;
    CNetSocket mScoketRaw;
    CNetSocket mScoketListener;

private:
    bool send();
    bool sendReset();
    void closeAll();
};

}//namespace net
}//namespace app

#endif // APP_CNETSYNPING_H
