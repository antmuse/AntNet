#include "CBase64Coder.h"


#define APP_BASE64_PAD	    '='
#define APP_BASE64DE_FIRST	'+'
#define APP_BASE64DE_LAST	'z'


namespace app {


//BASE 64 encode table
static const char AppBase64EncodeTable[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};


//ASCII order for BASE 64 decode, -1 in unused character
static const s8 AppBase64DecodeTable[] = {
    /* '+', ',', '-', '.', '/', '0', '1', '2', */
    62, -1, -1, -1, 63, 52, 53, 54,

    /* '3', '4', '5', '6', '7', '8', '9', ':', */
    55, 56, 57, 58, 59, 60, 61, -1,

    /* ';', '<', '=', '>', '?', '@', 'A', 'B', */
    -1, -1, -1, -1, -1, -1, 0, 1,

    /* 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', */
    2, 3, 4, 5, 6, 7, 8, 9,

    /* 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', */
    10, 11, 12, 13, 14, 15, 16, 17,

    /* 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', */
    18, 19, 20, 21, 22, 23, 24, 25,

    /* '[', '\', ']', '^', '_', '`', 'a', 'b', */
    -1, -1, -1, -1, -1, -1, 26, 27,

    /* 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', */
    28, 29, 30, 31, 32, 33, 34, 35,

    /* 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', */
    36, 37, 38, 39, 40, 41, 42, 43,

    /* 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', */
    44, 45, 46, 47, 48, 49, 50, 51
};


u32 CBase64Coder::encode(const u8* in, u32 inlen, s8* out) {
    u32 i, j;

    for(i = j = 0; i < inlen; i++) {
        s32 s = i % 3; 			/* from 6/gcd(6, 8) */

        switch(s) {
        case 0:
            out[j++] = AppBase64EncodeTable[(in[i] >> 2) & 0x3F];
            continue;
        case 1:
            out[j++] = AppBase64EncodeTable[((in[i - 1] & 0x3) << 4) + ((in[i] >> 4) & 0xF)];
            continue;
        case 2:
            out[j++] = AppBase64EncodeTable[((in[i - 1] & 0xF) << 2) + ((in[i] >> 6) & 0x3)];
            out[j++] = AppBase64EncodeTable[in[i] & 0x3F];
        }
    }

    /* move back */
    i -= 1;

    /* check the last and add padding */
    if((i % 3) == 0) {
        out[j++] = AppBase64EncodeTable[(in[i] & 0x3) << 4];
        out[j++] = APP_BASE64_PAD;
        out[j++] = APP_BASE64_PAD;
    } else if((i % 3) == 1) {
        out[j++] = AppBase64EncodeTable[(in[i] & 0xF) << 2];
        out[j++] = APP_BASE64_PAD;
    }

    return j;// finished;
}


u32 CBase64Coder::decode(const s8* in, u32 inlen, u8* out) {
    u32 i, j;

    for(i = j = 0; i < inlen; i++) {
        s32 c;
        s32 s = i % 4; 			/* from 8/gcd(6, 8) */

        if(in[i] == '=')
            return j;// finished;

        if(in[i] < APP_BASE64DE_FIRST || in[i] > APP_BASE64DE_LAST ||
            (c = AppBase64DecodeTable[in[i] - APP_BASE64DE_FIRST]) == -1)
            return j;// invalid input char;

        switch(s) {
        case 0:
            out[j] = ((u32) c << 2) & 0xFF;
            continue;
        case 1:
            out[j++] += ((u32) c >> 4) & 0x3;

            /* if not last char with padding */
            if(i < (inlen - 3) || in[inlen - 2] != '=')
                out[j] = ((u32) c & 0xF) << 4;
            continue;
        case 2:
            out[j++] += ((u32) c >> 2) & 0xF;

            /* if not last char with padding */
            if(i < (inlen - 2) || in[inlen - 1] != '=')
                out[j] = ((u32) c & 0x3) << 6;
            continue;
        case 3:
            out[j++] += (u8) c;
        }
    }

    return j; // finished;
}


} //namespace app




/*
int main(int argc, char** argv) {
    char* src = "base64test";
    const app::s32 strsize = 1 + ::strlen(src);
    int esize = app::CBase64Coder::getEncodeCacheSize(strsize);
    char* dest = new char[esize];
    esize = app::CBase64Coder::encode((app::u8*)src, strsize, dest);
    printf("src=%s/%d, encode=%s/%d\n", src, strsize, dest, esize);

    int dsize = app::CBase64Coder::getDecodeCacheSize(esize);
    char* decode = new char[dsize];
    dsize = app::CBase64Coder::decode(dest, esize, (app::u8*)decode);
    printf("decode=%s/%d, encode=%s/%d\n", decode, dsize, dest, esize);
    delete[] dest;
    delete[] decode;

    return 0;
    }
*/
