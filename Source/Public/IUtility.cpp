#include "IUtility.h"
#include <stdio.h>
#include "irrMath.h"

#if defined(APP_PLATFORM_WINDOWS) && defined(_MSC_VER) && (_MSC_VER > 1298)
#include <stdlib.h>
#define bswap_16(X) _byteswap_ushort(X)
#define bswap_32(X) _byteswap_ulong(X)
#elif defined(APP_PLATFORM_LINUX)
#include <byteswap.h>
#else
#define bswap_16(X) ((((X)&0xFF) << 8) | (((X)&0xFF00) >> 8))
#define bswap_32(X) ((((X)&0x000000FF)<<24) | (((X)&0xFF000000) >> 24) | (((X)&0x0000FF00) << 8) | (((X) &0x00FF0000) >> 8))
#endif



namespace irr {

	IUtility::IUtility(){
	}


	IUtility::~IUtility(){
	}


    u16 IUtility::swapByte(u16 num) {
        return bswap_16(num);
    }


    s16 IUtility::swapByte(s16 num) {
        return bswap_16(num);
    }


    u32 IUtility::swapByte(u32 num) {
        return bswap_32(num);
    }


    s32 IUtility::swapByte(s32 num) {
        return bswap_32(num);
    }


    f32 IUtility::swapByte(f32 num) {
        u32 tmp=IR(num); 
        tmp=bswap_32(tmp); 
        return (FR(tmp));
    }


    // prevent accidental byte swapping of chars
    APP_FORCE_INLINE u8  IUtility::swapByte(u8 num) {
        return num;
    }


    // prevent accidental byte swapping of chars
    APP_FORCE_INLINE s8  IUtility::swapByte(s8 num) {
        return num;
    }


    bool IUtility::isSmallEndian() {  
        u16 it = 0xFF00;  
        u8* c = (u8*) (&it);
        return ((c[0] == 0x00) && (c[1] == 0xFF));
    }  


	u32 IUtility::convertToHexString(const u8* iData, u32 iDataSize, c8* iResult, u32 iSize){
		if(iSize < iDataSize*2+1){
			return 0;
		}
		for(u32 i=0; i<iDataSize; ++i){
			snprintf(iResult, iSize, "%02X", *iData++);
			iResult+=2;
			iSize-=2;
		}
		return iDataSize*2;
	}


	u32 IUtility::convertToU8(const c8* iData, u32 iDataSize, u8* iResult, u32 iSize){
		if(iDataSize > iSize*2){
			iDataSize = iSize*2;
		}
		for(u32 i=0; i<iDataSize; i+=2){
			u8 a = convertToU8(*iData++);
			u8 b = convertToU8(*iData++);
			*iResult++ = a<<4 | b;
		}
		return iDataSize/2;
	}


	void IUtility::print(const u8* iData, u32 iSize) {
		for(u32 i=0; i<iSize; ++i){
			printf("%02X", iData[i]);
		}
	}

}// end namespace irr
