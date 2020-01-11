#include "HAtomicOperator.h"

#if defined( APP_PLATFORM_WINDOWS )
#include <Windows.h>
#include <intrin.h>

namespace irr {

void AppAtomicReadBarrier() {
    _ReadBarrier();
}

void AppAtomicWriteBarrier() {
    _WriteBarrier();
}

void AppAtomicReadWriteBarrier() {
    _ReadWriteBarrier();
}

void* AppAtomicFetchSet(void* value, void** iTarget) {
    return InterlockedExchangePointer(iTarget, value);
}

s32 AppAtomicFetchOr(s32 value, s32* iTarget) {
    return InterlockedOr((LONG*) iTarget, value);
}

s16 AppAtomicFetchOr(s16 value, s16* iTarget) {
    return InterlockedOr16((short*) iTarget, value);
}

s32 AppAtomicFetchXor(s32 value, s32* iTarget) {
    return InterlockedXor((LONG*) iTarget, value);
}

s16 AppAtomicFetchXor(s16 value, s16* iTarget) {
    return InterlockedXor16((short*) iTarget, value);
}

s32 AppAtomicFetchAnd(s32 value, s32* iTarget) {
    return InterlockedAnd((LONG*) iTarget, value);
}

s16 AppAtomicFetchAnd(s16 value, s16* iTarget) {
    return InterlockedAnd16((short*) iTarget, value);
}

s32 AppAtomicFetchAdd(s32 addValue, s32* iTarget) {
    return InterlockedExchangeAdd((LONG*) iTarget, addValue);
}


s32 AppAtomicIncrementFetch(s32* it) {
    return InterlockedIncrement((LONG*) it);
}


s32 AppAtomicDecrementFetch(s32* it) {
    return InterlockedDecrement((LONG*) it);
}


s32 AppAtomicFetchSet(s32 value, s32* iTarget) {
    return InterlockedExchange((LONG*) iTarget, value);
}

s64 AppAtomicFetchSet(s64 value, s64* iTarget) {
    return InterlockedExchange64(iTarget, value);
}

s32 AppAtomicFetchCompareSet(s32 newValue, s32 comparand, s32* iTarget) {
    return InterlockedCompareExchange((LONG*) iTarget, newValue, comparand);
}


//64bit functions---------------------------------------------
s64 AppAtomicIncrementFetch(s64* it) {
    return InterlockedIncrement64((LONG64*) it);
}


s64 AppAtomicDecrementFetch(s64* it) {
    return InterlockedDecrement64((LONG64*) it);
}


//16bit functions---------------------------------------------
s16 AppAtomicIncrementFetch(s16* it) {
    return InterlockedIncrement16((SHORT*) it);
}


s16 AppAtomicDecrementFetch(s16* it) {
    return InterlockedDecrement16((SHORT*) it);
}


s32 AppAtomicFetch(s32* iTarget) {
    return InterlockedExchangeAdd((LONG*) iTarget, 0);
}
s64 AppAtomicFetch(s64* iTarget) {
    return InterlockedExchangeAdd64(iTarget, 0);
}

} //end namespace irr

#elif defined( APP_PLATFORM_ANDROID ) || defined( APP_PLATFORM_LINUX )
namespace irr {
// in gcc >= 4.7:

void AppAtomicReadBarrier() {
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

void AppAtomicWriteBarrier() {
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

void AppAtomicReadWriteBarrier() {
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

s32 AppAtomicFetchAdd(s32 addValue, s32* iTarget) {
    return __atomic_fetch_add_4(iTarget, addValue, __ATOMIC_SEQ_CST);
}


void* AppAtomicFetchSet(void* iValue, void** iTarget) {
    return (void*)__atomic_exchange_8(iTarget, (unsigned long)iValue, __ATOMIC_SEQ_CST);
}


s32 AppAtomicIncrementFetch(s32* it) {
    return __atomic_add_fetch_4(it, 1, __ATOMIC_SEQ_CST);
}


s32 AppAtomicDecrementFetch(s32* it) {
    return __atomic_sub_fetch_4(it, 1, __ATOMIC_SEQ_CST);
}


s32 AppAtomicFetchSet(s32 value, s32* iTarget) {
    return __atomic_exchange_4(iTarget, value, __ATOMIC_SEQ_CST);
}

s64 AppAtomicFetchSet(s64 value, s64* iTarget) {
    return __atomic_exchange_8(iTarget, value, __ATOMIC_SEQ_CST);
}

s32 AppAtomicFetchCompareSet(s32 newValue, s32 comparand, s32* iTarget) {
    __atomic_compare_exchange_4(iTarget, &comparand, newValue, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return comparand;
}

s64 AppAtomicFetchCompareSet(s64 newValue, s64 comparand, s64* iTarget) {
    __atomic_compare_exchange_8(iTarget, &comparand, newValue, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return comparand;
}

s32 AppAtomicFetch(s32* iTarget) {
    return __atomic_load_4(iTarget, __ATOMIC_SEQ_CST);
}
s64 AppAtomicFetch(s64* iTarget) {
    return __atomic_load_8(iTarget, __ATOMIC_SEQ_CST);
}

} //end namespace irr
#endif //APP_PLATFORM_WINDOWS

