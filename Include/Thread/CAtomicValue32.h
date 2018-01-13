/**
*@file CAtomicValue32.h
*@brief This file defined a utility for 32bit atomic value.
*@author antmuse@live.cn
*@date 2014-09-22
*/

#ifndef APP_CATOMICVALUE32_H
#define APP_CATOMICVALUE32_H

#include "HConfig.h"
#include "irrTypes.h"

namespace irr {

/**
*@class CAtomicS32
*@brief A 32bit atomic value.
*/
class APP_ALIGN(32) CAtomicS32 {
public:
    CAtomicS32();

    CAtomicS32(const s32 val);

    CAtomicS32(const CAtomicS32& it);

    CAtomicS32& operator=(const CAtomicS32& it);

    ~CAtomicS32();

    s32 operator+(const CAtomicS32& it)const;
    s32 operator-(const CAtomicS32& it)const;

    s32 operator+(const s32 it)const;
    s32 operator+=(const s32 it);

    s32 operator-(const s32 it)const;
    s32 operator-=(const s32 it);

    s32 operator++();
    s32 operator++(const s32 it);

    s32 operator--();
    s32 operator--(const s32 it);

    s32 operator=(const s32 it);

    bool operator==(const s32 it)const;

    bool operator!=(const s32 it)const;

    bool operator>(const s32 it)const;

    bool operator>=(const s32 it)const;

    bool operator<(const s32 it)const;

    bool operator<=(const s32 it)const;

    s32 getValue()const {
        return mValue;
    }

protected:
    s32 mValue; //volatile
};


} //namespace irr

#endif	// APP_CATOMICVALUE32_H
