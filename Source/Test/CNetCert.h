#ifndef APP_CNETCERT_H
#define APP_CNETCERT_H


#include "CString.h"
#include "CNetPacket.h"
#include "mbedtls/ssl.h"
#include "mbedtls/certs.h"

namespace app {
namespace net {

class CNetCert {
public:
    CNetCert();

    ~CNetCert();

    const CNetPacket& getRawCert()const {
        return mPacket;
    }

    mbedtls_x509_crt& getCert() {
        return mCertCA;
    }

    const mbedtls_x509_crt& getCert()const {
        return mCertCA;
    }

    bool loadFromFile(const core::CPath& filenm);

    bool loadFromBuf(const void* buf = nullptr, u64 iSize = 0);

private:
    CNetPacket mPacket;
    mbedtls_x509_crt mCertCA;
};

}//namespace net
}//namespace app

#endif //APP_CNETCERT_H