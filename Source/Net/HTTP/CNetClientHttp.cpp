#include "CNetClientHttp.h"
#include "INetEventerHttp.h"
#include "IUtility.h"
#include "IAppLogger.h"
#include "CNetHttpRequest.h"


namespace irr {
namespace net {

CNetClientHttp::CNetClientHttp(INetClientSeniorTCP* hub) :
    mHub(hub),
    mSession(0),
    mReceiver(0) {
    mAddressRemote.setPort(80);
}


CNetClientHttp::~CNetClientHttp() {
    stop();
}


void CNetClientHttp::postData() {
    if(mReceiver) {
        mReceiver->onReceive(mResponse);
    }
}


void CNetClientHttp::onBody() {
    postData();

    switch(mResponse.getStatusCode()) {
    case EHS_MOVED_PERMANENTLY:
    case EHS_SEE_OTHER:
    case EHS_FOUND:
    {
        const core::stringc* loc = mResponse.getHead().getValue(EHHID_LOCATION);
        if(!loc) {
            break;
        }
        CNetHttpURL& url = (mRequest.getURL());
        u32 flag = url.setAndCheck(*loc);
        if(!flag) {
            break;
        }
        if(CNetHttpURL::EURL_NEW_HTTP & flag) {
            if(url.isHTTPS()) {
                onEvent(ENET_UNSUPPORT);
                //mSession->postDisconnect();
                break;
            }
        }
        if((CNetHttpURL::EURL_NEW_HOST) & flag) {
            mAddressRemote.setDomain(url.getHost().c_str());
        }
        if((CNetHttpURL::EURL_NEW_PORT) & flag) {
            mAddressRemote.setPort(url.getPort());
        }
        mRelocation = true;
        break;
    }
    }//switch
}


void CNetClientHttp::onReceive(CNetPacket& it) {
    const c8* pos = mResponse.import(it.getConstPointer(), it.getSize());
    if(!pos) return;

    if(mResponse.isFull()) {
        onBody();
        //mResponse.clear();
    }

    it.clear(pos);
}


void CNetClientHttp::onEvent(ENetEventType it) {
    if(ENET_CLOSED == it) {
        mSession = 0;
        if(mRelocation) {
            mRelocation = false;
            if(start()) {
                return;
            }
        } else {
            if(mResponse.getContentSize() == 0 && !mResponse.isKeepAlive()) {
                postData();
            }
        }
    }

    if(mReceiver) {
        mReceiver->onEvent(it);
    }
}


bool CNetClientHttp::request() {
    CNetPacket pack(256);
    mRequest.getBuffer(pack);
    return mSession->send(pack.getConstPointer(), (s32) pack.getSize());
}


bool CNetClientHttp::start() {
    if(mSession) {
        return true;
    }
    clear();
    mSession = mHub->getSession(this);
    if(mSession) {
        if(mSession->connect(mAddressRemote)) {
            return true;
        }
        mSession->setEventer(0);
        mSession = 0;
    }
    return false;
}


bool CNetClientHttp::stop() {
    if(mSession) {
        return mSession->disconnect();
    }
    return true;
}


void CNetClientHttp::clear() {
    mRelocation = false;
    mResponse.clear();
    mRequest.clear();
}

}// end namespace net
}// end namespace irr
