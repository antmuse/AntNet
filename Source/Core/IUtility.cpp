#include "IUtility.h"
#include <stdio.h>

#if defined(APP_USE_ICONV)
#include "iconv.h"
#endif

/*
#if defined(APP_PLATFORM_WINDOWS) && defined(_MSC_VER) && (_MSC_VER > 1298)
#include <stdlib.h>
#define bswap_16(X) _byteswap_ushort(X)
#define bswap_32(X) _byteswap_ulong(X)
#define bswap_64(X) _byteswap_uint64(X)
#elif defined(APP_PLATFORM_LINUX)
#include <byteswap.h>
#else
#define bswap_16(X) ((((X)&0xFF) << 8) | (((X)&0xFF00) >> 8))
#define bswap_32(X) ((((X)&0x000000FF)<<24) | (((X)&0xFF000000) >> 24) | (((X)&0x0000FF00) << 8) | (((X) &0x00FF0000) >> 8))
#define bswap_64(X) ( (((X)&0x00000000000000FFULL)<<56) | (((X)&0xFF00000000000000ULL)>>56)  \
                    | (((X)&0x000000000000FF00ULL)<<40) | (((X)&0x00FF000000000000ULL)>>40)  \
                    | (((X)&0x0000000000FF0000ULL)<<24) | (((X)&0x0000FF0000000000ULL)>>24)  \
                    | (((X)&0x00000000FF000000ULL)<< 8) | (((X)&0x000000FF00000000ULL)>> 8))
#endif
*/


#ifdef APP_PLATFORM_WINDOWS
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <shlwapi.h>
#endif

#ifdef APP_PLATFORM_LINUX
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#endif

#ifdef APP_PLATFORM_ANDROID
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#endif

namespace app {
namespace core {

core::CWString APP_LOCALE_DECIMAL_POINTS(L".");


#if defined(APP_USE_ICONV)

void* IUtility::m2Wchar = ((iconv_t)-1);
void* IUtility::m2UTF8 = ((iconv_t)-1);
void* IUtility::mWchar2UTF8 = ((iconv_t)-1);
void* IUtility::mUTF8ToWchar = ((iconv_t)-1);
void* IUtility::mGBK2UTF8 = ((iconv_t)-1);
void* IUtility::mGBK2Wchar = ((iconv_t)-1);


IUtility& IUtility::getInstance() {
    static IUtility it;
    return it;
}


IUtility::IUtility() {
#if defined(APP_PLATFORM_ANDROID)|| defined(APP_PLATFORM_LINUX)
    IUtility::m2Wchar = iconv_open("UCS-4-INTERNAL//IGNORE", "");
    IUtility::mGBK2Wchar = iconv_open("UCS-4-INTERNAL//IGNORE", "GB18030");
    IUtility::mWchar2UTF8 = iconv_open("utf-8//IGNORE", "UCS-4-INTERNAL");
    IUtility::mUTF8ToWchar = iconv_open("UCS-4-INTERNAL", "utf-8");
#elif defined(APP_PLATFORM_WINDOWS)
    IUtility::m2Wchar = iconv_open("UCS-2-INTERNAL", "");
    IUtility::mWchar2UTF8 = iconv_open("utf-8//IGNORE", "UCS-2-INTERNAL");
    IUtility::mUTF8ToWchar = iconv_open("UCS-2-INTERNAL//IGNORE", "utf-8");
    IUtility::mGBK2Wchar = iconv_open("UCS-2-INTERNAL//IGNORE", "GB18030");
#endif
    IUtility::m2UTF8 = iconv_open("utf-8//IGNORE", "");
    IUtility::mGBK2UTF8 = iconv_open("utf-8//IGNORE", "GB18030");
    APP_ASSERT(m2Wchar != ((libiconv_t)-1));
    APP_ASSERT(m2UTF8 != ((libiconv_t)-1));
    APP_ASSERT(mWchar2UTF8 != ((libiconv_t)-1));
    APP_ASSERT(mUTF8ToWchar != ((libiconv_t)-1));
    APP_ASSERT(mGBK2UTF8 != ((libiconv_t)-1));
    APP_ASSERT(mGBK2Wchar != ((libiconv_t)-1));
}


IUtility::~IUtility() {
    iconv_close(m2Wchar);
    iconv_close(m2UTF8);
    iconv_close(mUTF8ToWchar);
    iconv_close(mWchar2UTF8);
    iconv_close(mGBK2UTF8);
    iconv_close(mGBK2Wchar);
}


s32 IUtility::convert2Wchar(const s8* in, size_t inbytesleft, wchar_t* out, size_t outbytesleft) {
    APP_ASSERT(in && out);
    s32 ret = 0;
    s8* p_out = (s8*)out;
    outbytesleft = (outbytesleft - 1) << 1;//sizeof(wchar_t)=2 and reserve 1 for L'\0'
    ret = (s32)iconv(m2Wchar, &in, &inbytesleft, &p_out, &outbytesleft);
    *((wchar_t*)p_out) = L'\0';
    APP_ASSERT(0 == inbytesleft);
    return (s32)(0 == ret ? ((p_out - ((s8*)out)) >> 1) : 0);
}

s32 IUtility::convert2UTF8(const s8* in, size_t inbytesleft, s8* out, size_t outbytesleft) {
    APP_ASSERT(in && out);
    s8* p_out = out;
    outbytesleft = (outbytesleft - 1);//reserve 1 for '\0'
    s32 ret = (s32)iconv(m2UTF8, &in, &inbytesleft, &p_out, &outbytesleft);
    *p_out = '\0';
    APP_ASSERT(0 == inbytesleft);
    return (s32)(0 == ret ? (p_out - out) : 0);
}

s32 IUtility::convertGBK2UTF8(const s8* in, size_t inbytesleft, s8* out, size_t outbytesleft) {
    APP_ASSERT(in && out);
    s8* p_out = out;
    outbytesleft = (outbytesleft - 1);//reserve 1 for '\0'
    s32 ret = (s32)iconv(mGBK2UTF8, &in, &inbytesleft, &p_out, &outbytesleft);
    *p_out = '\0';
    APP_ASSERT(0 == inbytesleft);
    return (s32)(0 == ret ? (p_out - out) : 0);
}

s32 IUtility::convertGBK2Wchar(const s8* pChars, size_t inbytesleft, wchar_t* out, size_t outbytesleft) {
    outbytesleft = (outbytesleft - 1) << 1;//sizeof(wchar_t)=2 and reserve 1 for L'\0'
    s8* p_out = (s8*)out;
    size_t ret = iconv(mGBK2Wchar, &pChars, &inbytesleft, &p_out, &outbytesleft);
    *((wchar_t*)p_out) = L'\0';
    APP_ASSERT(0 == inbytesleft);
    return (s32)(0 == ret ? ((p_out - ((s8*)out)) >> 1) : 0);
}

s32 IUtility::convertUTF82Wchar(const s8* pChars, size_t inbytesleft, wchar_t* out, size_t outbytesleft) {
    outbytesleft = (outbytesleft - 1) << 1;//sizeof(wchar_t)=2 and reserve 1 for L'\0'
    s8* p_out = (s8*)out;
    size_t ret = iconv(mUTF8ToWchar, &pChars, &inbytesleft, &p_out, &outbytesleft);
    *((wchar_t*)p_out) = L'\0';
    APP_ASSERT(0 == inbytesleft);
    return (s32)(0 == ret ? ((p_out - ((s8*)out)) >> 1) : 0);
}

s32 IUtility::convertWchar2UTF8(const wchar_t* in, size_t inbytesleft, s8* out, size_t outbytesleft) {
    APP_ASSERT(in && out);
    s8* p_out = out;
    outbytesleft = (outbytesleft - 1);//reserve 1 for '\0'
    s32 ret = (s32)iconv(mWchar2UTF8, (const s8 * *)& in, &inbytesleft, &p_out, &outbytesleft);
    *p_out = '\0';
    APP_ASSERT(0 == inbytesleft);
    return (s32)(0 == ret ? (p_out - out) : 0);
}
#endif //APP_USE_ICONV


u32 AppConvertToHexString(const void* in, u32 inSize, s8* out, u32 outLen) {
    u32 ret = 0;
    if(in && out && outLen > 0) {
        ret = core::min_<u32>((outLen - 1) >> 1, inSize);
        const u8* iData = (const u8*)in;
        for(u32 i = 0; i < ret; ++i) {
            snprintf(out, 3, "%02X", *iData++);
            out += 2;
        }
        ret <<= 1;
        *out = '\0';
    }
    return ret;
}

u32 AppConvertToHexString(const void* in, u32 inSize, wchar_t* out, u32 outLen) {
    u32 ret = 0;
    if(in && out && outLen > 0) {
        ret = core::min_<u32>((outLen - 1) >> 1, inSize);
        const u8* iData = (const u8*)in;
        for(u32 i = 0; i < ret; ++i) {
            swprintf(out, 4, L"%02X", *iData++);
            out += 2;
        }
        ret <<= 1;
        out[ret] = L'\0';
    }
    return ret;
}


u32 AppConvertToU8(const s8* iData, u32 iDataSize, u8* iResult, u32 iSize) {
    if(iDataSize > iSize * 2) {
        iDataSize = iSize * 2;
    }
    for(u32 i = 0; i < iDataSize; i += 2) {
        u8 a = AppConvertToU8(*iData++);
        u8 b = AppConvertToU8(*iData++);
        *iResult++ = a << 4 | b;
    }
    return iDataSize >> 1;
}


void AppPrintToHexString(const void* iData, u32 iSize) {
    const u8* buffer = (const u8*)iData;
    for(u32 i = 0; i < iSize; ++i) {
        printf("%02X", buffer[i]);
    }
}


void AppPrintToHexText(const void* buffer, u32 len) {
    printf("////////////////////////////////////////////////////////////////////////////////////\n");
    const u32 max = 88;
    const s8* const buf = (const s8*)buffer;
    u32 i, j, k;
    s8 binstr[max];
    for(i = 0; i < len; i++) {
        if(0 == (i % 16)) {
            snprintf(binstr, max, "%08x: ", i);
            snprintf(binstr, max, "%s %02x", binstr, buf[i]);
        } else if(15 == (i % 16)) {
            snprintf(binstr, max, "%s %02x", binstr, buf[i]);
            snprintf(binstr, max, "%s  ", binstr);
            for(j = i - 15; j <= i; j++) {
                snprintf(binstr, max, "%s%c", binstr, ('!' < buf[j] && buf[j] <= '~') ? buf[j] : '.');
            }
            printf("%s\n", binstr);
        } else {
            snprintf(binstr, max, "%s %02x", binstr, buf[i]);
        }
    }
    if(0 != (i % 16)) {
        k = 16 - (i % 16);
        for(j = 0; j < k; j++) {
            snprintf(binstr, max, "%s   ", binstr);
        }
        snprintf(binstr, max, "%s  ", binstr);
        k = 16 - k;
        for(j = i - k; j < i; j++) {
            snprintf(binstr, max, "%s%c", binstr, ('!' < buf[j] && buf[j] <= '~') ? buf[j] : '.');
        }
        printf("%s\n", binstr);
    }
    printf("////////////////////////////////////////////////////////////////////////////////////\n");
}


const s8* AppSkipFlag(const s8* iStart, const s8* const iEnd, const s8 iLeft, const s8 iRight) {
    iStart = AppGoNextFlag(iStart, iEnd, iLeft);
    ++iStart;
    s32 it = 1;
    while(iStart != iEnd) {
        if(*iStart == iLeft) {
            ++it;
        } else if(*iStart == iRight) {
            --it;
            if(it <= 0) {
                ++iStart;
                break;
            }
        }
        ++iStart;
    }
    return iStart;
}


const s8* AppGoNextFlag(const s8* iStart, const s8* const iEnd, const s8 ichar) {
    while((iStart < iEnd) && (*iStart != ichar)) {
        ++iStart;
    }
    return iStart;
}


const s8* AppGoBackFlag(const s8* iStart, const s8* iEnd, const s8 it) {
    while((iStart > iEnd) && (*iStart != it)) {
        --iStart;
    }
    return iStart;
}


const s8* AppGoNextLine(const s8* iStart, const s8* const iEnd) {
    while(iStart < iEnd) {
        if(*iStart == '\n') {
            ++iStart;
            break;
        }
        ++iStart;
    }
    return iStart;
}


const s8* AppGoAndCopyNextWord(s8* iOutBuffer, const s8* iStart, u32 outBufLength, const s8* iEnd, bool acrossNewlines) {
    iStart = AppGoNextWord(iStart, iEnd, acrossNewlines);
    return iStart += AppCopyWord(iOutBuffer, iStart, outBufLength, iEnd);
}


u32 AppCopyWord(s8* iOutBuffer, const s8* const iStart, u32 outBufLength, const s8* const iEnd) {
    if(!outBufLength) {
        return 0;
    }
    if(!iStart) {
        *iOutBuffer = 0;
        return 0;
    }
    u32 i = 0;
    while(iStart[i]) {
        if(core::isspace(iStart[i]) || &(iStart[i]) == iEnd) {
            break;
        }
        ++i;
    }
    u32 length = core::min_(i, outBufLength - 1);
    for(u32 j = 0; j < length; ++j) {
        iOutBuffer[j] = iStart[j];
    }
    iOutBuffer[i] = 0;
    return length;
}


const s8* AppGoFirstWord(const s8* iStart, const s8* const iEnd, bool acrossNewlines) {
    if(acrossNewlines) {
        while((iStart < iEnd) && core::isspace(*iStart)) {
            ++iStart;
        }
    } else {
        while((iStart < iEnd) && core::isspace(*iStart) && (*iStart != '\n')) {
            ++iStart;
        }
    }
    return iStart;
}


const s8* AppGoNextWord(const s8* iStart, const s8* const iEnd, bool acrossNewlines) {
    while((iStart < iEnd) && !core::isspace(*iStart)) {
        ++iStart;
    }
    return AppGoFirstWord(iStart, iEnd, acrossNewlines);
}


bool AppCreatePath(const core::CPath& iPath) {
    bool ret = true;
#if defined(APP_PLATFORM_WINDOWS)
    core::CPath realpath;
    core::CPath directory(APP_STR(".\\"));
    core::splitFilename(iPath, &realpath);
    realpath.replace(APP_STR('\\'), APP_STR('/'));
    s32 start = 0;
    s32 end = realpath.size();
    while(start < end) {
        s32 pos = realpath.findNext(APP_STR('/'), start);
        pos = (pos > 0 ? pos : end - 1);
        directory += realpath.subString(start, pos - start);
        directory.append(APP_STR('\\'));
        start = pos + 1;
        if(::PathFileExists(directory.c_str())) {
            continue;
        }
        if(FALSE == ::CreateDirectory(directory.c_str(), 0)) {
            ret = false;
            break;
        }
    }
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    //CEngine::getInstance().get->add(iPath.c_str());
    DIR * pDirect = opendir(iPath.c_str());
    if(0 == pDirect) {
        if(0 == ::mkdir(iPath.c_str(), 0777)) {
            //CLogger::logCritical("IUtility::createPath", "new path=%s", iPath.c_str());
            //CEngine::mPrinter->addW(L"created new path");
        } else {
            ret = false;
            //CEngine::mPrinter->addW(L"path create error");
        }
    } else {
        ::closedir(pDirect);
        //CEngine::mPrinter->addW(L"Old path already there");
    }
#endif
    return ret;
}

///**
// * Returns a list of files/directories in the supplied directory.
// * Used internally for auto-installation of plugins.
// */
//static array<CString> getFilesInDirectory(CString path) {
//    array<CString> fileList;
//#ifdef APP_PLATFORM_WINDOWS
//    CString search = path + "\\" + CString("*.*");
//    WIN32_FIND_DATA info;
//    HANDLE h = FindFirstFile(search.c_str(), &info);
//    if (h != INVALID_HANDLE_VALUE) {
//        do {
//            if (!(strcmp(info.cFileName, ".") == 0 || strcmp(info.cFileName, "..") == 0)) {
//                fileList.push_back(info.cFileName);
//            }
//        } while (FindNextFile(h, &info));
//        FindClose(h);
//    }
//#endif
//
//#ifdef APP_PLATFORM_LINUX
//    DIR *d;
//    struct dirent *dir;
//    d = opendir(path.c_str());
//    if (d) {
//        while ((dir = readdir(d)) != NULL) {
//            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
//                continue;
//            }
//            if (dir->d_type == DT_DIR) continue;
//            fileList.push_back(dir->d_name);
//        }
//        closedir(d);
//    }
//#endif
//    return fileList;
//};


} //namespace core
} //namespace app
