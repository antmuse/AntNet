#include "CNetHttpHead.h"
#include "CNetPacket.h"

namespace app {
namespace net {

CNetHttpHead::CNetHttpHead() {

}


CNetHttpHead::~CNetHttpHead() {

}


void CNetHttpHead::getBuffer(CNetPacket& out) const {
    THttpHeadNode* nd;
    for(THttpHeadIterator it = mMapHead.getIterator(); !it.atEnd(); it++) {
        nd = it.getNode();
        const core::CString& key = nd->getKey();
        const core::CString& value = nd->getValue();
        out.addBuffer(key.c_str(), key.size());
        out.add(':');
        out.addBuffer(value.c_str(), value.size());
        out.addBuffer("\r\n", 2);
    }
}


} //namespace net
} //namespace app
