/**
 * @file IUtility.h
 * @brief   The utility functions of project.
 * @author Antmuse|Email:antmuse@live.cn
 * @version 0.3.1.6
 * @date 2010年8月28日, 下午2:26
 */

#ifndef APP_IUTILITY_H
#define	APP_IUTILITY_H

#include "HConfig.h"
#include "path.h"


namespace irr {
/**
 *@brief The tool class for engine.
 *@class IUtility
 */
class IUtility {
public:
    static IUtility& getInstance();

    static u16 swapByte(u16 num);
    static s16 swapByte(s16 num);
    static u32 swapByte(u32 num);
    static s32 swapByte(s32 num);
    static f32 swapByte(f32 num);
    static s64 swapByte(s64 num);
    static u64 swapByte(u64 num);
    //swap any size of byte;
    static void swapByte(u8* start, u32 size) {
        APP_ASSERT(start && size > 1);
        //if(start && size > 1) {
        u8* end = start + size - 1;
        u8 tmp;
        for(; start < end;) {
            tmp = *start;
            *start++ = *end;
            *end-- = tmp;
        }
    }
    ///prevent accidental swapping of chars
    APP_FORCE_INLINE static u8  swapByte(u8  num);
    ///prevent accidental byte swapping of chars
    APP_FORCE_INLINE static s8  swapByte(s8  num);

    static bool isSmallEndian();

    /* encode 8 bits unsigned int */
    APP_FORCE_INLINE static c8* encodeU8(const u8 c, c8* p) {
        *(u8*) p++ = c;
        return p;
    }

    /* decode 8 bits unsigned int */
    APP_FORCE_INLINE static const c8* decodeU8(const c8* p, u8 *c) {
        *c = *(u8*) p++;
        return p;
    }

    /* encode 16 bits unsigned int (lsb) */
    APP_FORCE_INLINE static c8* encodeU16(const u16 w, c8* p) {
#ifdef APP_ENDIAN_BIG
        *(u8*) (p + 0) = (w & 255);
        *(u8*) (p + 1) = (w >> 8);
#else
        *(u16*) (p) = w;
#endif
        p += 2;
        return p;
    }

    /* decode 16 bits unsigned int (lsb) */
    APP_FORCE_INLINE static const c8* decodeU16(const c8* p, u16* w) {
#ifdef APP_ENDIAN_BIG
        *w = *(const u8*) (p + 1);
        *w = *(const u8*) (p + 0) + (*w << 8);
#else
        *w = *(const u16*) p;
#endif
        p += 2;
        return p;
    }

    /* encode 32 bits unsigned int (lsb) */
    APP_FORCE_INLINE static c8* encodeU32(const u32 l, c8* p) {
#ifdef APP_ENDIAN_BIG
        *(u8*) (p + 0) = (u8) ((l >> 0) & 0xff);
        *(u8*) (p + 1) = (u8) ((l >> 8) & 0xff);
        *(u8*) (p + 2) = (u8) ((l >> 16) & 0xff);
        *(u8*) (p + 3) = (u8) ((l >> 24) & 0xff);
#else
        *(u32*) p = l;
#endif
        p += 4;
        return p;
    }

    /* decode 32 bits unsigned int (lsb) */
    APP_FORCE_INLINE static const c8* decodeU32(const c8* p, u32* l) {
#ifdef APP_ENDIAN_BIG
        *l = *(const u8*) (p + 3);
        *l = *(const u8*) (p + 2) + (*l << 8);
        *l = *(const u8*) (p + 1) + (*l << 8);
        *l = *(const u8*) (p + 0) + (*l << 8);
#else 
        *l = *(const u32*) p;
#endif
        p += 4;
        return p;
    }

    /**
    *@brief Print data buffer.
    *@param iData Data to print.
    *@param iSize  Length of data buffer.
    */
    static void print(const u8* iData, u32 iSize);



    /**
    *@brief Convert a buffer to a 16 hexadecimal string.
    *@note iSize >= iDataSize*2+1
    *@param iData The buffer to read from.
    *@param iDataSize  The length of read buffer.
    *@param iResult The cache to write in, cache length must >= (read buffer length * 2 + 1).
    *@param iSize  The length of write cache.
    */
    static u32 convertToHexString(const u8* iData, u32 iDataSize, c8* iResult, u32 iSize);

    /**
    *@brief Convert a 16 hexadecimal string to a buffer.
    *@note iSize >= iDataSize/2
    *@param iData The buffer to read from.
    *@param iDataSize  The length of read buffer.
    *@param iResult The cache to write in, cache length must >= (read buffer length / 2).
    *@param iSize  The length of write cache.
    */
    static u32 convertToU8(const c8* iData, u32 iDataSize, u8* iResult, u32 iSize);

    /**
    *@brief Convert a hexadecimal char to 1 byte.
    *@param ch The hexadecimal char.
    *@return The converted byte.
    */
    static u8 convertToU8(c8 ch) {
        if(ch >= '0' && ch <= '9') {
            return (ch - '0');
        }
        if(ch >= 'A' && ch <= 'F') {
            return (ch - 'A' + 10);
        }
        if(ch >= 'a' && ch <= 'f') {
            return (ch - 'a' + 10);
        }
        return 0;
    }


    static const c8* goNextLine(const c8* iStart, const c8* const iEnd);

    // combination of goNextWord followed by copyWord
    static const c8* goAndCopyNextWord(c8* pOutBuf, const c8* iStart, u32 outBufLength, const c8 * const iEnd, bool acrossNewlines = false);

    // returns a pointer to the first printable character after the first non-printable
    static const c8* goNextWord(const c8* iStart, const c8* const iEnd, bool acrossNewlines = true);

    // returns a pointer to the first printable character available in the buffer
    static const c8* goFirstWord(const c8* iStart, const c8* const iEnd, bool acrossNewlines = true);

    // copies the current word from the inBuf to the outBuf
    static u32 copyWord(c8* outBuf, const c8* iStart, u32 outBufLength, const c8* const iEnd);

    const c8* goBackFlag(const c8* iStart, const c8* iEnd, const c8 it);

    /**
    *@brief go to next mark ichar
    */
    static const c8* goNextFlag(const c8* iBuf, const c8* const iEnd, const c8 ichar);

    /**
    *@brief skip the codes that marked with ichar & ichar2
    */
    static const c8* skipFlag(const c8* iStart, const c8* const iEnd, const c8 leftFlag, const c8 rightFlag);

    static bool createPath(const io::path& iPath);

    static s32 ascToWchar(const c8* pChars, size_t* inLen, wchar_t* pWChars, size_t* outLen);
    /**
    * @brief Copy maxlen chars from pSrc to pDest.
    * @param pSrc The source string.
    * @param pDest The dest string.
    * @param maxLen The max length of copy.
    * @param div The divide char for break.
    * @return Number of copied.
    */
    static s32 copyCharN(const c8* pSrc, c8* pDest, s32 maxLen) {
        if(pSrc == 0 || pDest == 0) {
            return 0;
        }
        --maxLen;
        s32 indx = 0;
        while(*pSrc != 0 && indx < maxLen) {
            *pDest++ = *pSrc++;
            ++indx;
        }
        *pDest = 0;
        return indx;
    }


    static void toUpper(c8* iSrc) {
        while(*iSrc) {
            if(*iSrc >= 'a' && *iSrc <= 'z') {
                *iSrc -= 32;
            }
            ++iSrc;
        }
    }


    static void toLower(c8* iSrc) {
        while(*iSrc) {
            if(*iSrc >= 'A' && *iSrc <= 'Z') {
                *iSrc += 32;
            }
            ++iSrc;
        }
    }


    static void toLower(c8* iSrc, u32 size) {
        for(u32 i = 0; i < size; ++i) {
            if(*(iSrc) >= 'A' && *(iSrc) <= 'Z') {
                *(iSrc) += 32;
            }
            ++iSrc;
        }
    }


    static bool equalsn(const c8* str1, const c8* str2, u32 size) {
        for(u32 i = 0; i < size; ++i) {
            if(*str1++ != *str2++) {
                return false;
            }
        }
        return true;
    }


    APP_INLINE static bool isChar(const c8 in, const c8 smallChar) {
        return ((in == smallChar) || (in == (smallChar - 32)));
    }

private:
    IUtility();
    ~IUtility();
};

} //namespace irr
#endif	/* APP_IUTILITY_H */