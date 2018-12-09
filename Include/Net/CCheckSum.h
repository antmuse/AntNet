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

    CCheckSum(const CCheckSum& other) :
        mHaveTail(other.mHaveTail),
        mSum(other.mSum),
        mLeftover(other.mLeftover) {
    }

    CCheckSum& operator=(const CCheckSum& other) {
        mSum = other.mSum;
        mLeftover = other.mLeftover;
        mHaveTail = other.mHaveTail;
        return *this;
    }

    /**
    * @brief Clear this checksum and reuse it.
    */
    void clear() {
        mSum = 0;
        mLeftover = 0;
        mHaveTail = false;
    }

    /**
    * @brief Append buffer into this checksum.
    * @param buffer The buffer.
    * @param iSize The buffer length.
    */
    void add(const void* buffer, s32 iSize);

    /**
    * @brief Get result, and still can append buffers into this checksum in future.
    * @return The result.
    */
    u16 get()const;

    /**
    * @brief Get result, and should not append buffers into this checksum in future.
    * @return The result.
    */
    u16 finish();

private:
    u32 mSum;
    mutable u16 mLeftover;
    bool  mHaveTail;
};

}//namespace irr
#endif //APP_CCHECKSUM_H
