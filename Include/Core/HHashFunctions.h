#ifndef APP_HHASHFUNCTIONS_H
#define APP_HHASHFUNCTIONS_H


#include "HConfig.h"

namespace app {
namespace core {
typedef u32(*AppHashCallback32)(const void*, u64);
typedef u64(*AppHashCallback64)(const void*, u64);

//////////////////////////////////sip hash//////////////////////////////////
u64 AppHashSIP(const void* key, u64 len);

u64 AppCaseHashSIP(const void* buf, u64 len);

//@param seed 16 bytes array
void AppSetHashSeedSIP(const void* seed);

//@return seed 16 bytes array, can't delete/free
void* AppGetHashSeedSIP();


//////////////////////////////////murmur hash//////////////////////////////////
void AppSetHashSeedMurmur(const u32 seed);

u32& AppGetHashSeedMurmur();

//@param out 128 bits cache
void AppHashMurmur128(const void* buf, u64 len, void* out);
u64 AppHashMurmur64(const void* buf, u64 len);
u32 AppHashMurmur32(const void* buf, u64 len);


//////////////////////////////////other hash//////////////////////////////////

u32 AppHashRS(const void* iBuffer, u64 iSize);

u32 AppHashJS(const void* iBuffer, u64 iSize);

/**
*@brief P. J. Weinberger Hash Function
*/
u32 AppHashPJW(const void* iBuffer, u64 iSize);

u32 AppHashELF(const void* iBuffer, u64 iSize);

u32 AppHashBKDR(const void* iBuffer, u64 iSize);

u32 AppHashSDBM(const void* iBuffer, u64 iSize);

u32 AppHashDJB(const void* iBuffer, u64 iSize);

u32 AppHashDEK(const void* iBuffer, u64 iSize);

u32 AppHashBP(const void* iBuffer, u64 iSize);

u32 AppHashFNV(const void* iBuffer, u64 iSize);

u32 AppHashAP(const void* iBuffer, u64 iSize);

} //namespace core
} //namespace app

#endif //APP_HHASHFUNCTIONS_H
