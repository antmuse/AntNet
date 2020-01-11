/*
 **************************************************************************
 *                                                                        *
 *          General Purpose Hash Function Algorithms Library              *
 *                                                                        *
 * Author: Arash Partow - 2002                                            *
 * URL: http://www.partow.net                                             *
 * URL: http://www.partow.net/programming/hashfunctions/index.html        *
 *                                                                        *
 * Copyright notice:                                                      *
 * Free use of the General Purpose Hash Function Algorithms Library is    *
 * permitted under the guidelines and in accordance with the MIT License. *
 * http://www.opensource.org/licenses/MIT                                 *
 *                                                                        *
 **************************************************************************
*/


#include "HHashFunctions.h"

namespace irr {
namespace core {

u32 AppHashRS(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 b = 378551;
    u32 a = 63689;
    u32 hash = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash = hash * a + (*str);
        a = a * b;
    }
    return hash;
}


u32 AppHashJS(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = 1315423911;
    for(u64 i = 0; i < len; str++, i++) {
        hash ^= ((hash << 5) + (*str) + (hash >> 2));
    }
    return hash;
}


u32 AppHashPJW(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    const u32 BitsInUnsignedInt = (u32)(sizeof(u32) * 8);
    const u32 ThreeQuarters = (u32)((BitsInUnsignedInt * 3) / 4);
    const u32 OneEighth = (u32)(BitsInUnsignedInt / 8);
    const u32 HighBits = 0xFFFFFFFFU << (BitsInUnsignedInt - OneEighth);
    u32 hash = 0;
    u32 test = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash = (hash << OneEighth) + (*str);
        if((test = hash & HighBits) != 0) {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }
    return hash;
}


u32 AppHashELF(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = 0;
    u32 x = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash = (hash << 4) + (*str);
        if((x = hash & 0xF0000000L) != 0) {
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }
    return hash;
}


u32 AppHashBKDR(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 seed = 131; /* 31 131 1313 13131 131313 etc.. */
    u32 hash = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash = (hash * seed) + (*str);
    }
    return hash;
}


u32 AppHashSDBM(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash = (*str) + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}


u32 AppHashDJB(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = 5381;
    for(u64 i = 0; i < len; str++, i++) {
        hash = ((hash << 5) + hash) + (*str);
    }
    return hash;
}


u32 AppHashDEK(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = (u32)len;
    for(u64 i = 0; i < len; str++, i++) {
        hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);
    }
    return hash;
}


u32 AppHashBP(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash = hash << 7 ^ (*str);
    }
    return hash;
}


u32 AppHashFNV(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    const u32 fnv_prime = 0x811C9DC5;
    u32 hash = 0;
    for(u64 i = 0; i < len; str++, i++) {
        hash *= fnv_prime;
        hash ^= (*str);
    }
    return hash;
}


u32 AppHashAP(const void* buf, u64 len) {
    const u8* str = reinterpret_cast<const u8*>(buf);
    u32 hash = 0xAAAAAAAA;

    for(u64 i = 0; i < len; str++, i++) {
        hash ^= ((i & 1) == 0) ? ((hash << 7) ^ (*str) * (hash >> 3)) :
            (~((hash << 11) + ((*str) ^ (hash >> 5))));
    }
    return hash;
}


} //namespace core
} //namespace irr
