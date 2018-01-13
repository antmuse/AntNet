#include "CNetHttpRequest.h"
#include "HNetConfig.h"
#include "irrMath.h"
#include "IUtility.h"
#include "CNetPacket.h"


namespace irr {
namespace net {

CNetHttpRequest::CNetHttpRequest() :
    mHttpVersion("1.1"),
    mMethod("GET") {
    core::stringc tmp("Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1667.0 Safari/537.36");
    mHead.setValue(EHHID_USER_AGENT, tmp);
    tmp = "*/*";
    mHead.setValue(EHRID_ACCEPT, tmp);
    tmp = "Keep-Alive";
    //tmp = "close";
    mHead.setValue(EHRID_CONNECTION, tmp);
}


CNetHttpRequest::~CNetHttpRequest() {

}


void CNetHttpRequest::getBuffer(CNetPacket& out)const {
    out.addBuffer(mMethod.c_str(), mMethod.size());
    out.addBuffer(" ", 1);
    out.addBuffer(getPath().c_str(), getPath().size());
    out.addBuffer(" HTTP/", 6);
    out.addBuffer(mHttpVersion.c_str(), mHttpVersion.size());
    out.addBuffer("\r\n", 2);
    out.addBuffer(AppHttpHeadIDName[EHHID_HOST], AppHttpHeadIDNameSize[EHHID_HOST]);
    out.addBuffer(": ", 2);
    out.addBuffer(getHost().c_str(), getHost().size());
    out.addBuffer("\r\n", 2);
    mHead.getBuffer(out);
    out.addBuffer("\r\n", 2);
}


void CNetHttpRequest::clear() {

}

} //namespace net
} //namespace irr