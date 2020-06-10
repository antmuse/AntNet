#include "CNetCert.h"
#include "CFileReader.h"


namespace irr {
namespace net {


CNetCert::CNetCert() {
    mbedtls_x509_crt_init(&mCertCA);
}

CNetCert::~CNetCert() {
    mbedtls_x509_crt_free(&mCertCA);
}

bool CNetCert::loadFromFile(const io::path& filenm) {
    io::CFileReader rer;
    if (!rer.openFile(filenm)) {
        return false;
    }
    u64 fsz = rer.getFileSize();

    mPacket.setUsed((u32)fsz);
    if (fsz == rer.read(mPacket.getPointer(), fsz)) {
        s32 ret = mbedtls_x509_crt_parse(&mCertCA, (const u8*)mPacket.getPointer(), fsz);
        //ret = mbedtls_x509_crt_parse(&mCertCA, (const u8*)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
        if (ret < 0) {
            APP_ASSERT(0);
            mPacket.setUsed(0);
        }
    } else {
        mPacket.setUsed(0);
    }

    return mPacket.getSize() > 0;
}


bool CNetCert::loadFromBuf(const void* buf, u64 iSize) {
    if (!buf) {
        buf = mbedtls_test_cas_pem;
        iSize = mbedtls_test_cas_pem_len;
    }
    mPacket.setUsed(0);
    s32 ret = mbedtls_x509_crt_parse(&mCertCA, (const u8*)buf, iSize);
    if (ret >= 0) {
        mPacket.setUsed((u32)iSize);
        memcpy(mPacket.getPointer(), buf, iSize);
    }
    return mPacket.getSize() > 0;
}

}//namespace net
}//namespace irr
