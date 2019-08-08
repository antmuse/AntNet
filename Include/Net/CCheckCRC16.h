#ifndef APP_CCHECKCRC16_H
#define APP_CCHECKCRC16_H

#include "irrTypes.h"

namespace irr {

/*
* @brief CRC16 implementation according to CCITT standards.
*/
class CCheckCRC16 {
public:
    CCheckCRC16();

    ~CCheckCRC16();

    u16 add(const void* buffer, u32 size);

    void clear() {
        mSum = 0;
    }

    u16 getResult()const {
        return mSum;
    }

private:
    u16 mSum;
};

}//namespace irr
#endif //APP_CCHECKCRC16_H
