/**
*@file CCheckSum.h
*@brief CCheckSum generates and validates checksums.
*/

#ifndef APP_CCHECKSUM_H
#define APP_CCHECKSUM_H

#include "irrTypes.h"

namespace irr {

/**
* @brief Checksum for TCP/IP head.
* Checksum routine for Internet Protocol family headers (Portable Version).
*
* This routine is very heavily used in the network
* code and should be modified for each CPU to be as fast as possible.
*/
class CCheckSum {
public:
    CCheckSum() :
        mHaveTail(false),
        mSum(0),
        mLeftover(0) {
    }

    void clear() {
        mSum = 0;
        mLeftover = 0;
        mHaveTail = false;
    }

    void set(u32 sum, u8 leftover) {
        mSum = sum;
        *(u8*) (&mLeftover) = leftover;
        mHaveTail = (0 != leftover);
    }

    void add(const void* buffer, s32 iSize);

    u16 get();

private:
    u32 mSum;
    u16 mLeftover;
    bool  mHaveTail;
};


///////////////////outdate code/////////////////////
class COutdateCheckSum {
public:

    COutdateCheckSum() :
        mLeftover(0),
        mSum(0) {
    }

    void clear() {
        mSum = 0;
        mLeftover = 0;
    }

    void set(u32 it, u8 leftover) {
        mSum = it;
        mLeftover = leftover;
    }

    /**
    * @brief Add buffer into checksum.
    * @param buffer Buffer pointer.
    * @param iSize Buffer size.
    * @note Buffer size should less than 2^16, else maybe be overflow.
    *  Fortunately, none net packet size more than 2^16-1.
    */
    void add(const void* buffer, u32 iSize);

    /**
    * @brief Get checkSum result.
    */
    u16 get() const {
        u32 ret = (mSum >> 16) + (mSum & 0x0000FFFF);
        //ret += (ret >> 16);
        while(0 != (ret & 0xFFFF0000)) {
            ret = (ret >> 16) + (ret & 0x0000FFFF);
        }
        return (u16) (~ret);
    }

protected:
    u32 mSum;
    u8 mLeftover;
};


/* 32-bit crc16 */
static u32 APP_CRC16(const c8* data, s32 len) {
    u32 sum;
    for(sum = 0; len; len--) {
        /*
        * gcc 2.95.2 x86 and icc 7.1.006 compile
        * that operator into the single "rol" opcode,
        * msvc 6.0sp2 compiles it into four opcodes.
        */
        sum = sum >> 1 | sum << 31;
        sum += *data++;
    }
    return sum;
}


}//namespace irr
#endif //APP_CCHECKSUM_H
