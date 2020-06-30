#ifndef APP_CBASE64CODER_H
#define APP_CBASE64CODER_H

#include "HConfig.h"

namespace app {
 

#define APP_BASE64_ENCODE_OUT_SIZE(s)	(((s) + 2) / 3 * 4)
#define APP_BASE64_DECODE_OUT_SIZE(s)	(((s)) / 4 * 3)


class CBase64Coder {
public:

    static u32 getDecodeCacheSize(u32 inputSize) {
        return (((inputSize)) / 4 * 3);
    }

    static u32 getEncodeCacheSize(u32 inputSize) {
        return (((inputSize) +2) / 3 * 4);
    }

    /**
    *@brief encode base64 string.
    *@param inputData Input buffer.
    *@param inLength Input buffer size.
    *@note output cache size should be no less than result of getEncodeCacheSize().
    *@return How many bytes were written.
    */
    static u32 encode(const u8* inputData, u32 inLength, s8* outputData);

    /**
    *@brief decode base64 string.
    *@param inputData Input buffer.
    *@param inLength Input buffer size.
    *@note output cache size should be no less than result of getDecodeCacheSize().
    *@return How many bytes were written.
    */
    static u32 decode(const s8* inputData, u32 inLength, u8* outputData);


protected:
    CBase64Coder(){ }
    ~CBase64Coder(){ }
};

} //namespace app

#endif //APP_CBASE64CODER_H