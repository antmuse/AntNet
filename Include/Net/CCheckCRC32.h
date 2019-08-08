#ifndef APP_CCHECKCRC32_H
#define APP_CCHECKCRC32_H

#include "irrTypes.h"

namespace irr {

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

}//namespace irr
#endif //APP_CCHECKCRC32_H
