#include "CSuperFastHash.h"
#include <stdio.h>
#include <stdlib.h>


namespace irr {

#if !defined(_WIN32)
#include <stdint.h>
#endif

#undef get16bits

#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const u16 *) (d)))
#else
#define get16bits(d) ((((u32)(((const u8 *)(d))[1])) << 8)\
	+(u32)(((const u8 *)(d))[0]) )
#endif

static const s32 INCREMENTAL_READ_BLOCK = 65536;


u32 SuperFastHash(const c8* data, s32 length) {
    // All this is necessary or the hash does not match SuperFastHashIncremental
    s32 bytesRemaining = length;
    u32 lastHash = length;
    s32 offset = 0;
    while(bytesRemaining >= INCREMENTAL_READ_BLOCK) {
        lastHash = SuperFastHashIncremental(data + offset, INCREMENTAL_READ_BLOCK, lastHash);
        bytesRemaining -= INCREMENTAL_READ_BLOCK;
        offset += INCREMENTAL_READ_BLOCK;
    }
    if(bytesRemaining > 0) {
        lastHash = SuperFastHashIncremental(data + offset, bytesRemaining, lastHash);
    }
    return lastHash;

    //	return SuperFastHashIncremental(data,len,len);
}


u32 SuperFastHashIncremental(const c8* data, s32 len, u32 lastHash) {
    u32 hash = (u32) lastHash;
    u32 tmp;
    s32 rem;

    if(len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for(; len > 0; len--) {
        hash += get16bits(data);
        tmp = (get16bits(data + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        data += 2 * sizeof(u16);
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch(rem) {
    case 3: hash += get16bits(data);
        hash ^= hash << 16;
        hash ^= data[sizeof(u16)] << 18;
        hash += hash >> 11;
        break;
    case 2: hash += get16bits(data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return (u32) hash;

}


u32 SuperFastHashFile(const c8* filename) {
    FILE *fp = fopen(filename, "rb");
    if(fp == 0)
        return 0;
    u32 hash = SuperFastHashFilePtr(fp);
    fclose(fp);
    return hash;
}


u32 SuperFastHashFilePtr(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    s32 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    s32 bytesRemaining = length;
    u32 lastHash = length;
    char readBlock[INCREMENTAL_READ_BLOCK];
    while(bytesRemaining >= (s32) sizeof(readBlock)) {
        fread(readBlock, sizeof(readBlock), 1, fp);
        lastHash = SuperFastHashIncremental(readBlock, (s32) sizeof(readBlock), lastHash);
        bytesRemaining -= (s32) sizeof(readBlock);
    }
    if(bytesRemaining > 0) {
        fread(readBlock, bytesRemaining, 1, fp);
        lastHash = SuperFastHashIncremental(readBlock, bytesRemaining, lastHash);
    }
    return lastHash;
}


}//namespace irr