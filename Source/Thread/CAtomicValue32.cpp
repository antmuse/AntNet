#include "CAtomicValue32.h"
#include "HAtomicOperator.h"

namespace irr {

CAtomicS32::CAtomicS32() :
    mValue(0) {
}

CAtomicS32::CAtomicS32(const CAtomicS32& it) :
    mValue(it.mValue) {
}

CAtomicS32::CAtomicS32(const s32 val) :
    mValue(val) {
}

CAtomicS32::~CAtomicS32() {
}

CAtomicS32& CAtomicS32::operator=(const CAtomicS32& it) {
    AppAtomicFetchSet(it.mValue, &mValue);
    return *this;
}

s32 CAtomicS32::operator+(const CAtomicS32& it)const {
    //return AppAtomicFetchAdd(0, &mValue) + AppAtomicFetchAdd(0, &it.mValue);
    return mValue + it.mValue;
}

s32 CAtomicS32::operator-(const CAtomicS32& it)const {
    //return AppAtomicFetchAdd(0, &mValue) - AppAtomicFetchAdd(0, &it.mValue);
    return mValue - it.mValue;
}

s32 CAtomicS32::operator+(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) + it;
    return mValue + it;
}

s32 CAtomicS32::operator+=(const s32 it) {
    AppAtomicFetchAdd(it, &mValue);
    return mValue;
}

s32 CAtomicS32::operator-(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) - it;
    return mValue - it;
}

s32 CAtomicS32::operator-=(const s32 it) {
    AppAtomicFetchAdd(-it, &mValue);
    return mValue;
}

s32 CAtomicS32::operator++() {
    return AppAtomicIncrementFetch(&mValue);
}

s32 CAtomicS32::operator++(const s32 it) {
    s32 ret = mValue;
    AppAtomicIncrementFetch(&mValue);
    return ret;
}

s32 CAtomicS32::operator--() {
    return AppAtomicDecrementFetch(&mValue);
}

s32 CAtomicS32::operator--(const s32 it) {
    s32 ret = mValue;
    AppAtomicDecrementFetch(&mValue);
    return ret;
}

s32 CAtomicS32::operator=(const s32 it) {
    AppAtomicFetchSet(it, &mValue);
    return mValue;
}

bool CAtomicS32::operator==(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) == it;
    return mValue == it;
}

bool CAtomicS32::operator!=(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) != it;
    return mValue != it;
}

bool CAtomicS32::operator>(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) > it;
    return mValue > it;
}

bool CAtomicS32::operator>=(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) >= it;
    return mValue >= it;
}

bool CAtomicS32::operator<(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) < it;
    return mValue < it;
}

bool CAtomicS32::operator<=(const s32 it)const {
    //return AppAtomicFetchAdd(0, &mValue) <= it;
    return mValue <= it;
}

} //namespace irr
