#ifndef APP_IUTILITY_H
#define APP_IUTILITY_H

#include "HConfig.h"
#include "irrTypes.h"

namespace irr {

	class IUtility {
	public:
		IUtility();
		~IUtility();

		static u16 swapByte(u16 num);
		static s16 swapByte(s16 num);
		static u32 swapByte(u32 num);
		static s32 swapByte(s32 num);
		static f32 swapByte(f32 num);
		///prevent accidental swapping of chars
		APP_FORCE_INLINE static u8  swapByte(u8  num);
		///prevent accidental byte swapping of chars
		APP_FORCE_INLINE static s8  swapByte(s8  num);

		static bool isSmallEndian();

		/* encode 8 bits unsigned int */
		APP_FORCE_INLINE static s8* encodeU8(const u8 c, s8* p){
			*(u8*)p++ = c;
			return p;
		}

		/* decode 8 bits unsigned int */
		APP_FORCE_INLINE static const s8* decodeU8(const s8* p, u8 *c){
			*c = *(u8*)p++;
			return p;
		}

		/* encode 16 bits unsigned int (lsb) */
		APP_FORCE_INLINE static s8* encodeU16(const u16 w, s8* p){
#ifdef APP_ENDIAN_BIG
			*(u8*)(p + 0) = (w & 255);
			*(u8*)(p + 1) = (w >> 8);
#else
			*(u16*)(p) = w;
#endif
			p += 2;
			return p;
		}

		/* decode 16 bits unsigned int (lsb) */
		APP_FORCE_INLINE static const s8* decodeU16(const s8* p, u16* w){
#ifdef APP_ENDIAN_BIG
			*w = *(const u8*)(p + 1);
			*w = *(const u8*)(p + 0) + (*w << 8);
#else
			*w = *(const u16*)p;
#endif
			p += 2;
			return p;
		}

		/* encode 32 bits unsigned int (lsb) */
		APP_FORCE_INLINE static s8* encodeU32(const u32 l, s8* p){
#ifdef APP_ENDIAN_BIG
			*(u8*)(p + 0) = (u8)((l >>  0) & 0xff);
			*(u8*)(p + 1) = (u8)((l >>  8) & 0xff);
			*(u8*)(p + 2) = (u8)((l >> 16) & 0xff);
			*(u8*)(p + 3) = (u8)((l >> 24) & 0xff);
#else
			*(u32*)p = l;
#endif
			p += 4;
			return p;
		}

		/* decode 32 bits unsigned int (lsb) */
		APP_FORCE_INLINE static const s8* decodeU32(const s8 *p, u32 *l){
#ifdef APP_ENDIAN_BIG
			*l = *(const u8*)(p + 3);
			*l = *(const u8*)(p + 2) + (*l << 8);
			*l = *(const u8*)(p + 1) + (*l << 8);
			*l = *(const u8*)(p + 0) + (*l << 8);
#else 
			*l = *(const u32*)p;
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
		static u32 convertToHexString(const u8* iData, u32 iDataSize, c8* iResult, u32 iSize) ;

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
			if (ch >= '0' && ch <= '9'){
				return (ch - '0');
			}
			if (ch >= 'A' && ch <= 'F'){
				return (ch - 'A' + 10); 
			}
			if (ch >= 'a' && ch <= 'f'){
				return (ch - 'a' + 10); 
			}
			return 0;
		}

	private:

	};


}// end namespace irr
#endif //APP_IUTILITY_H
