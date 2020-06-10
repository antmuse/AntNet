//#include "CNetClientHttp.h"
//#include "INetEventerHttp.h"
//#include "IUtility.h"
//#include "IAppLogger.h"
//#include "CNetHttpRequest.h"
//
//
//namespace irr {
//namespace net {
//
//CNetClientHttp::CNetClientHttp(CNetServiceTCP* hub) :
//    mHub(hub),
//    mSession(0),
//    mReceiver(0) {
//    mAddressRemote.setPort(80);
//}
//
//
//CNetClientHttp::~CNetClientHttp() {
//    stop();
//}
//
//
//void CNetClientHttp::postData() {
//    if(mReceiver) {
//        mReceiver->onReceive(mResponse);
//    }
//}
//
//
//void CNetClientHttp::onBody() {
//    postData();
//
//    switch(mResponse.getStatusCode()) {
//    case EHS_MOVED_PERMANENTLY:
//    case EHS_SEE_OTHER:
//    case EHS_FOUND:
//    {
//        const core::stringc* loc = mResponse.getHead().getValue(EHHID_LOCATION);
//        if(!loc) {
//            break;
//        }
//        CNetHttpURL& url = (mRequest.getURL());
//        u32 flag = url.setAndCheck(*loc);
//        if(!flag) {
//            break;
//        }
//        if(CNetHttpURL::EURL_NEW_HTTP & flag) {
//            if(url.isHTTPS()) {
//                //onEvent(ENET_UNSUPPORT);
//                //mSession->postDisconnect();
//                break;
//            }
//        }
//        if((CNetHttpURL::EURL_NEW_HOST) & flag) {
//            mAddressRemote.setDomain(url.getHost().c_str());
//        }
//        if((CNetHttpURL::EURL_NEW_PORT) & flag) {
//            mAddressRemote.setPort(url.getPort());
//        }
//        mRelocation = true;
//        break;
//    }
//    }//switch
//}
//
//
//s32 CNetClientHttp::onConnect(u32 sessionID,
//    const CNetAddress& local, const CNetAddress& remote) {
//    s32 ret = 0;
//    if(mReceiver) {
//        //mReceiver->onConnect(sessionID, sock, local, remote);
//    }
//    return ret;
//}
//
//s32 CNetClientHttp::onDisconnect(u32 sessionID,
//    const CNetAddress& local, const CNetAddress& remote) {
//    s32 ret = 0;
//    mSession = 0;
//    if(mRelocation) {
//        mRelocation = false;
//        if(start()) {
//            return ret;
//        }
//    } else {
//        if(mResponse.getContentSize() == 0 && !mResponse.isKeepAlive()) {
//            postData();
//        }
//    }
//    return ret;
//}
//
//s32 CNetClientHttp::onSend(u32 sessionID, void* buffer, s32 size, s32 result) {
//    s32 ret = 0;
//    if(mReceiver) {
//        //mReceiver->onConnect(sessionID, sock, local, remote);
//    }
//    return ret;
//}
//
//s32 CNetClientHttp::onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size) {
//    s32 ret = 0;
//    const c8* pos = mResponse.import((c8*) buffer, size);
//    if(!pos) return ret;
//
//    if(mResponse.isFull()) {
//        onBody();
//        //mResponse.clear();
//    }
//
//    ret = (s32) (pos - ((c8*) buffer));
//
//    if(mReceiver) {
//        //mReceiver->onConnect(sessionID, sock, local, remote);
//    }
//    return ret;
//}
//
//
//bool CNetClientHttp::request() {
//    CNetPacket pack(256);
//    mRequest.getBuffer(pack);
//    return mHub->send(mSession, pack.getConstPointer(), (s32) pack.getSize());
//}
//
//
//bool CNetClientHttp::start() {
//    if(mSession) {
//        return true;
//    }
//    clear();
//    mSession = mHub->connect(mAddressRemote, this);
//    return mSession > 0;
//}
//
//
//bool CNetClientHttp::stop() {
//    return mSession ? mHub->disconnect(mSession) : true;
//}
//
//
//void CNetClientHttp::clear() {
//    mRelocation = false;
//    mResponse.clear();
//    mRequest.clear();
//}
//
//}// end namespace net
//}// end namespace irr
