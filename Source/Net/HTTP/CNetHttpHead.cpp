#include "CNetHttpHead.h"
#include "CNetPacket.h"

namespace irr {
namespace net {

CNetHttpHead::CNetHttpHead() {

}


CNetHttpHead::~CNetHttpHead() {

}


void CNetHttpHead::getBuffer(CNetPacket& out) const {
    THttpHeadNode* nd;
    EHttpHeadID key;
    for(THttpHeadIterator it = mMapHead.getIterator(); !it.atEnd(); it++) {
        nd = it.getNode();
        key = nd->getKey();
        core::stringc& value = nd->getValue();
        out.addBuffer(AppHttpHeadIDName[key], AppHttpHeadIDNameSize[key]);
        out.addBuffer(": ", 2);
        out.addBuffer(value.c_str(), value.size());
        out.addBuffer("\r\n", 2);
    }
}


} //namespace net
} //namespace irr
