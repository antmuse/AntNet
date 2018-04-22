// SYNPing类：实现对目标主机指定端口的SYN探测和扫描

#ifndef APP_CNETSYNPING_H
#define APP_CNETSYNPING_H

#include "HNetHeader.h"
#include "CNetSocket.h"


namespace irr {
namespace net {



class CNetSynPing {
public:
    CNetSynPing();
    virtual ~CNetSynPing();

    bool init();

    //实现Ping操作
    //0: 主机不存在
    //1: 主机存在但没监听指定端口
    //2: 主机存在并监听指定端口
    s32 ping(const c8* remoteIP, u16 remotePort);

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
}//namespace irr

#endif // APP_CNETSYNPING_H
