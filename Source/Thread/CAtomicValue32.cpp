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

s32 CAtomicS32::operator()()const {
    return AppAtomicFetch(&mValue);
}

CAtomicS32& CAtomicS32::operator=(const CAtomicS32& it) {
    AppAtomicFetchSet(it.mValue, &mValue);
    return *this;
}

s32 CAtomicS32::operator+(const CAtomicS32& it)const {
    return AppAtomicFetch(&mValue) + AppAtomicFetch(&it.mValue);
    //return mValue + it.mValue;
}

s32 CAtomicS32::operator-(const CAtomicS32& it)const {
    return AppAtomicFetch(&mValue) - AppAtomicFetch(&it.mValue);
    //return mValue - it.mValue;
}

s32 CAtomicS32::operator+(const s32 it)const {
    return AppAtomicFetch(&mValue) + it;
    //return mValue + it;
}

s32 CAtomicS32::operator+=(const s32 it) {
    AppAtomicFetchAdd(it, &mValue);
    return mValue;
}

s32 CAtomicS32::operator-(const s32 it)const {
    return AppAtomicFetch(&mValue) - it;
    //return mValue - it;
}

s32 CAtomicS32::operator-=(const s32 it) {
    AppAtomicFetchAdd(-it, &mValue);
    return mValue;
}

s32 CAtomicS32::operator++() {
    return AppAtomicIncrementFetch(&mValue);
}

s32 CAtomicS32::operator++(const s32 it) {
    return AppAtomicIncrementFetch(&mValue) - 1;
}

s32 CAtomicS32::operator--() {
    return AppAtomicDecrementFetch(&mValue);
}

s32 CAtomicS32::operator--(const s32 it) {
    return AppAtomicDecrementFetch(&mValue) + 1;
}

s32 CAtomicS32::operator=(const s32 it) {
    AppAtomicFetchSet(it, &mValue);
    return mValue;
}

bool CAtomicS32::operator==(const s32 it)const {
    return AppAtomicFetch(&mValue) == it;
    //return mValue == it;
}

bool CAtomicS32::operator!=(const s32 it)const {
    return AppAtomicFetch(&mValue) != it;
    //return mValue != it;
}

bool CAtomicS32::operator>(const s32 it)const {
    return AppAtomicFetch(&mValue) > it;
    //return mValue > it;
}

bool CAtomicS32::operator>=(const s32 it)const {
    return AppAtomicFetch(&mValue) >= it;
    //return mValue >= it;
}

bool CAtomicS32::operator<(const s32 it)const {
    return AppAtomicFetch(&mValue) < it;
    //return mValue < it;
}

bool CAtomicS32::operator<=(const s32 it)const {
    return AppAtomicFetch(&mValue) <= it;
    //return mValue <= it;
}

} //namespace irr
