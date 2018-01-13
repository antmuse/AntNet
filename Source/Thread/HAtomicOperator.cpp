#include "HAtomicOperator.h"

////#define App_CompilerBarrier()        do { asm volatile ("":::"memory"); } while (0)
//#define App_ReadCompilerBarrier()      do { __asm__ __volatile__ ("" : : : "memory"); } while (0)
//#define App_WriteCompilerBarrier()     do { __asm__ __volatile__ ("" : : : "memory"); } while (0)
//#define App_CompilerBarrier()          do { __asm__ __volatile__ ("" : : : "memory"); } while (0)
//
//#define App_ReadMemoryBarrier()        do { __sync_synchronize(); } while (0)
//#define App_WriteMemoryBarrier()       do { __sync_synchronize(); } while (0)
//#define App_MemoryBarrier()            do { __sync_synchronize(); } while (0)
//#define App_FullMemoryBarrier()        do { __sync_synchronize(); } while (0)
//
//#define App_CPU_ReadMemoryBarrier()    do { __asm__ __volatile__ ("lfence" : : : "memory"); } while (0)
//#define App_CPU_WriteMemoryBarrier()   do { __asm__ __volatile__ ("sfence" : : : "memory"); } while (0)
//#define App_CPU_MemoryBarrier()        do { __asm__ __volatile__ ("mfence" : : : "memory"); } while (0)



#if defined( APP_PLATFORM_WINDOWS )
#include <Windows.h>
#include <intrin.h>

namespace irr {

void AppAtomicReadBarrier() {
    ::_ReadBarrier();
}

void AppAtomicWriteBarrier() {
    ::_WriteBarrier();
}

void AppAtomicReadWriteBarrier() {
    ::_ReadWriteBarrier();
}

void* AppAtomicFetchSet(void* value, void** iTarget) {
    return ::InterlockedExchangePointer(iTarget, value);
}

s32 AppAtomicFetchOr(s32 value, s32* iTarget) {
    return ::InterlockedOr((LONG*) iTarget, value);
}

s32 AppAtomicFetchAnd(s32 value, s32* iTarget) {
    return ::InterlockedAnd((LONG*) iTarget, value);
}


s32 AppAtomicFetchAdd(s32 addValue, s32* iTarget) {
    return ::InterlockedExchangeAdd((LONG*) iTarget, addValue);
}


s32 AppAtomicIncrementFetch(s32* it) {
    return ::InterlockedIncrement((LONG*) it);
}


s32 AppAtomicDecrementFetch(s32* it) {
    return ::InterlockedDecrement((LONG*) it);
}


s32 AppAtomicFetchSet(s32 value, s32* iTarget) {
    return ::InterlockedExchange((LONG*) iTarget, value);
}


s32 AppAtomicFetchCompareSet(s32 newValue, s32 comparand, s32* iTarget) {
    return ::InterlockedCompareExchange((LONG*) iTarget, newValue, comparand);
}


//64bit functions---------------------------------------------
s64 AppAtomicIncrementFetch(s64* it) {
    return ::InterlockedIncrement64((LONG64*) it);
}


s64 AppAtomicDecrementFetch(s64* it) {
    return ::InterlockedDecrement64((LONG64*) it);
}


//16bit functions---------------------------------------------
s16 AppAtomicIncrementFetch(s16* it) {
    return ::InterlockedIncrement16((SHORT*) it);
}


s16 AppAtomicDecrementFetch(s16* it) {
    return ::InterlockedDecrement16((SHORT*) it);
}


} //end namespace irr

#elif defined( APP_PLATFORM_ANDROID ) || defined( APP_PLATFORM_LINUX )
namespace irr {

//__sync_lock_release();
void AppAtomicReadBarrier() {
    ::__sync_synchronize();
}

void AppAtomicWriteBarrier() {
    ::__sync_synchronize();
}

void AppAtomicReadWriteBarrier() {
    ::__sync_synchronize();
}

u32 AppAtomicFetchAdd(u32 addValue, u32* iTarget) {
    return ::__sync_fetch_and_add(iTarget, addValue);
}


u32 AppAtomicIncrementFetch(u32* it) {
    return ::__sync_add_and_fetch(it, 1);
}


u32 AppAtomicDecrementFetch(u32* it) {
    return ::__sync_sub_and_fetch(it, 1);
}


u32 AppAtomicFetchSet(u32 value, u32* iTarget) {
    return ::__sync_lock_test_and_set(iTarget, value);
}


u32 AppAtomicFetchCompareSet(u32 newValue, u32 comparand, u32* iTarget) {
    return ::__sync_lock_test_and_set(iTarget, newValue, comparand);
}


} //end namespace irr
#endif //APP_PLATFORM_WINDOWS

