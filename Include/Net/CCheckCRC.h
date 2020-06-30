#ifndef APP_CCHECKCRC_H
#define APP_CCHECKCRC_H

#include "HConfig.h"

namespace app {

/*
* @brief CRC16 implementation according to CCITT standards.
*/
class CCheckCRC16 {
public:

    CCheckCRC16() : mResult(0) {
    }

    ~CCheckCRC16() {
    }

    u16 add(const void* buffer, u32 size);

    void clear() {
        mResult = 0;
    }

    u16 getResult()const {
        return mResult;
    }

private:
    u16 mResult;
};


class CCheckCRC32 {
public:
    CCheckCRC32() : mResult(0xFFFFFFFF) {
    }
    ~CCheckCRC32() {
    }

    void clear() {
        mResult = 0xFFFFFFFF;
    }

    u32 getResult()const {
        return mResult ^ 0xFFFFFFFF;
    }

    u32 add(const void* buffer, u64 iSize);

private:
    u32 mResult;
};


class CCheckCRC64 {
public:
    CCheckCRC64() : mResult(0) {
    }
    ~CCheckCRC64() {
    }

    void clear() {
        mResult = 0;
    }

    u64 getResult()const {
        return mResult;
    }

    u64 add(const void* buffer, u64 iSize);

private:
    u64 mResult;
};

}//namespace app
#endif //APP_CCHECKCRC_H
