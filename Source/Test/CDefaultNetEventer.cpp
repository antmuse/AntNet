#include "CDefaultNetEventer.h"
#include "CNetClientSeniorTCP.h"
#include "CThread.h"
#include "IAppLogger.h"
#include "CNetPacket.h"
#include "INetSession.h"

namespace irr {
namespace net {

CDefaultNetEventer::CDefaultNetEventer() :
    mAutoConnect(false),
    mHub(0),
    mSession(0) {
}


CDefaultNetEventer::~CDefaultNetEventer() {
}



s32 CDefaultNetEventer::onEvent(SNetEvent& iEvent) {
    static u32 count = 0;
    const s32 packsz = 4 * 1024;
    s32 ret = 0;
    CNetPacket pack(packsz);

    IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent",
        "[%u] on [%s]", count, AppNetEventTypeNames[iEvent.mType]);

    switch(iEvent.mType) {
    case ENET_RECEIVED:
    {
        for(; iEvent.mInfo.mData.mSize >= packsz; iEvent.mInfo.mData.mSize -= packsz) {
            IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[received size=%u:%u=%s]",
                iEvent.mInfo.mData.mSize,
                *(u32*) iEvent.mInfo.mData.mBuffer,
                ((u32*) iEvent.mInfo.mData.mBuffer) + 1
            );
            ret += packsz;
        }
        break;
    }
    case ENET_LINKED:
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[%u,%s:%u->%s:%u]",
            iEvent.mInfo.mSession.mSocket->getValue(),
            iEvent.mInfo.mSession.mAddressLocal->getIPString(),
            iEvent.mInfo.mSession.mAddressLocal->getPort(),
            iEvent.mInfo.mSession.mAddressRemote->getIPString(),
            iEvent.mInfo.mSession.mAddressRemote->getPort()
        );

        iEvent.mInfo.mSession.mContext->setEventer(this);
        break;

    case ENET_CONNECTED:
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[%u,%s:%u->%s:%u]",
            iEvent.mInfo.mSession.mSocket->getValue(),
            iEvent.mInfo.mSession.mAddressLocal->getIPString(),
            iEvent.mInfo.mSession.mAddressLocal->getPort(),
            iEvent.mInfo.mSession.mAddressRemote->getIPString(),
            iEvent.mInfo.mSession.mAddressRemote->getPort()
        );
        //break;

    case ENET_SENT:
    {
        pack.clear(0x7FFFFFFF);
        pack.add(++count);
        pack.addBuffer("Hey,server", (u32) 11);
        mSession->send(pack.getPointer(), pack.getAllocatedSize());
        break;
    }

    case ENET_CLOSED:
        count = 0;
        IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[%u,%s:%u->%s:%u]",
            iEvent.mInfo.mSession.mSocket->getValue(),
            iEvent.mInfo.mSession.mAddressLocal->getIPString(),
            iEvent.mInfo.mSession.mAddressLocal->getPort(),
            iEvent.mInfo.mSession.mAddressRemote->getIPString(),
            iEvent.mInfo.mSession.mAddressRemote->getPort()
        );
        if(mAutoConnect) {
            mSession = mHub->getSession(this);
            if(mSession) {
                mSession->connect(*iEvent.mInfo.mSession.mAddressRemote);
            } else {
                IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onEvent", "[can't got session now-----]");
            }
        }
        break;
    }//switch

    return ret;
}


}//namespace net
}//namespace irr