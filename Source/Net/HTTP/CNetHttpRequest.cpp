#include "CNetHttpRequest.h"
#include "HNetConfig.h"
#include "AppMath.h"
#include "IUtility.h"
#include "CNetPacket.h"


namespace app {
namespace net {

CNetHttpRequest::CNetHttpRequest() :
    mHttpVersion("1.1"),
    mMethod("GET") {
    //core::CString tmp("Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1667.0 Safari/537.36");
    //mHead.setValue("User-Agent", tmp);
    //tmp = "*/*";
    //mHead.setValue("Accept", tmp);
}


CNetHttpRequest::~CNetHttpRequest() {

}


void CNetHttpRequest::getBuffer(CNetPacket& out)const {
    core::CString tmps;
    out.setUsed(0);
    out.addBuffer(mMethod.c_str(), mMethod.size());
    out.add(' ');
    mURL.getPath(tmps);
    out.addBuffer(tmps.c_str(), tmps.size());
    out.addBuffer(" HTTP/", 6);
    out.addBuffer(mHttpVersion.c_str(), mHttpVersion.size());
    out.addBuffer("\r\n", 2);
    out.addBuffer("Host", sizeof("Host") - 1);
    out.addBuffer(": ", 2);
    mURL.getHost(tmps);
    out.addBuffer(tmps.c_str(), tmps.size());
    out.addBuffer("\r\n", 2);
    mHead.getBuffer(out);
    out.addBuffer("\r\n", 2);
}


void CNetHttpRequest::clear() {
    mHead.clear();
}

} //namespace net
} //namespace app