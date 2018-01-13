#include "CDefaultNetEventer.h"
#include "CNetClientSeniorTCP.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "INetSession.h"

namespace irr {
namespace net {

CDefaultNetEventer::CDefaultNetEventer() :
    mHub(0),
    mSession(0) {
}


CDefaultNetEventer::~CDefaultNetEventer() {
}


void CDefaultNetEventer::onReceive(net::CNetPacket& pack) {
    core::array<c8> items(16);
    //pack.seek(0);
    u32 sz = 0;
    u32 node = 0;
    u32 id;
    for(;;) {
        if(pack.getSize() <= (sz + sizeof(u32))) {
            break;
        }
        node = pack.readU32();
        if(pack.getSize() < (sz + node)) {
            break;
        }
        sz += node;
        id = pack.readU32();
        pack.readString(items);
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onReceive", "[%u:%s]", id, items.pointer());
    }
    pack.clear(sz);
}


void CDefaultNetEventer::onEvent(ENetEventType iEvent) {
    //static core::stringc req("Hey,server");
    static CNetPacket pack;
    static u32 count = 0;

    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent",
        "on [%s]", AppNetEventTypeNames[iEvent]);

    switch(iEvent) {
    case ENET_CONNECTED:
    case ENET_SENT:
    {
        pack.setUsed(sizeof(u32));
        pack.add(++count);
        pack.add("Hey,server", (u32) 11);
        pack.setU32(0, pack.getSize());
        mSession->send(pack.getPointer(), pack.getSize());
        break;
    }
    }//switch
}


}//namespace net
}//namespace irr