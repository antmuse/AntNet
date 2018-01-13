/**
*@file CCheckSum.h
*@brief CCheckSum Generates and validates checksums.
*/

#ifndef APP_CCHECKSUM_H
#define APP_CCHECKSUM_H

#include "irrTypes.h"

namespace irr {

/// Generates and validates checksums
class CCheckSum {
public:

    CCheckSum() {
        clear();
    }

    void clear() {
        mSum = 0;
        r = 55665;
        c1 = 52845;
        c2 = 22719;
    }

    void add(u32 it);

    void add(u16 it);

    void add(u8* buffer, u32 length);

    void add(u8 it);

    u32 get() {
        return mSum;
    }

protected:
    u16 r;
    u16 c1;
    u16 c2;
    u32 mSum;
};

}//namespace irr
#endif //APP_CCHECKSUM_H
