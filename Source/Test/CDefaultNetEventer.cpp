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



s32 CDefaultNetEventer::onEvent(SNetEvent& iEvent) {
    s32 ret = 0;
    //static core::stringc req("Hey,server");
    CNetPacket pack(8 * 1024);
    static u32 count = 0;

    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent",
        "[%u] on [%s]", count, AppNetEventTypeNames[iEvent.mType]);

    switch(iEvent.mType) {
    case ENET_RECEIVED:
    {
        ret = iEvent.mInfo.mData.mSize;
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[received size=%u:%s]",
            iEvent.mInfo.mData.mSize, iEvent.mInfo.mData.mBuffer);
        break;
    }
    case ENET_LINKED:
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[%u,%s:%u->%s:%u]",
            iEvent.mInfo.mSession.mSocket->getValue(),
            iEvent.mInfo.mSession.mAddressRemote->getIPString(),
            iEvent.mInfo.mSession.mAddressRemote->getPort(),
            iEvent.mInfo.mSession.mAddressLocal->getIPString(),
            iEvent.mInfo.mSession.mAddressLocal->getPort());
        break;

    case ENET_CONNECTED:
    case ENET_SENT:
    {
        pack.setUsed(sizeof(u32));
        pack.add(++count);
        pack.add("Hey,server", (u32) 11);
        pack.setU32(0, pack.getSize());
        mSession->send(pack.getPointer(), pack.getAllocatedSize());
        break;
    }

    case ENET_CLOSED:
        count = 0;
        break;
    }//switch

    return ret;
}


}//namespace net
}//namespace irr