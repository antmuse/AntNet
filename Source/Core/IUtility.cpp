#include "IUtility.h"
#include <stdio.h>
//#include "irrMath.h"

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

namespace irr {
namespace core {

u32 AppConvertToHexString(const u8* iData, u32 iDataSize, c8* iResult, u32 iSize) {
    if(iSize < iDataSize * 2 + 1) {
        return 0;
    }
    for(u32 i = 0; i < iDataSize; ++i) {
        snprintf(iResult, iSize, "%02X", *iData++);
        iResult += 2;
        iSize -= 2;
    }
    return iDataSize * 2;
}


u32 AppConvertToU8(const c8* iData, u32 iDataSize, u8* iResult, u32 iSize) {
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
    const u8* buffer = ( const u8*) iData;
    for(u32 i = 0; i < iSize; ++i) {
        printf("%02X", buffer[i]);
    }
}


void AppPrintToHexText(const void* buffer, u32 len) {
    printf("////////////////////////////////////////////////////////////////////////////////////\n");
    const u32 max = 88;
    const c8* const buf = ( const c8*) buffer;
    u32 i, j, k;
    c8 binstr[max];
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


const c8* AppSkipFlag(const c8* iStart, const c8* const iEnd, const c8 iLeft, const c8 iRight) {
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


const c8* AppGoNextFlag(const c8* iStart, const c8* const iEnd, const c8 ichar) {
    while((iStart < iEnd) && (*iStart != ichar)) {
        ++iStart;
    }
    return iStart;
}


const c8* AppGoBackFlag(const c8* iStart, const c8* iEnd, const c8 it) {
    while((iStart > iEnd) && (*iStart != it)) {
        --iStart;
    }
    return iStart;
}


const c8* AppGoNextLine(const c8* iStart, const c8* const iEnd) {
    while(iStart < iEnd) {
        if(*iStart == '\n') {
            ++iStart;
            break;
        }
        ++iStart;
    }
    return iStart;
}


const c8* AppGoAndCopyNextWord(c8* iOutBuffer, const c8* iStart, u32 outBufLength, const c8* iEnd, bool acrossNewlines) {
    iStart = AppGoNextWord(iStart, iEnd, acrossNewlines);
    return iStart += AppCopyWord(iOutBuffer, iStart, outBufLength, iEnd);
}


u32 AppCopyWord(c8* iOutBuffer, const c8* const iStart, u32 outBufLength, const c8* const iEnd) {
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


const c8* AppGoFirstWord(const c8* iStart, const c8* const iEnd, bool acrossNewlines) {
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


const c8* AppGoNextWord(const c8* iStart, const c8* const iEnd, bool acrossNewlines) {
    while((iStart < iEnd) && !core::isspace(*iStart)) {
        ++iStart;
    }
    return AppGoFirstWord(iStart, iEnd, acrossNewlines);
}


bool AppCreatePath(const io::path& iPath) {
    bool ret = true;
#if defined(APP_PLATFORM_WINDOWS)
    io::path realpath;
    io::path directory(_IRR_TEXT(".\\"));
    core::splitFilename(iPath, &realpath);
    realpath.replace(_IRR_TEXT('\\'), _IRR_TEXT('/'));
    s32 start = 0;
    s32 end = realpath.size();
    while(start < end) {
        s32 pos = realpath.findNext(_IRR_TEXT('/'), start);
        pos = (pos > 0 ? pos : end - 1);
        directory += realpath.subString(start, pos - start);
        directory.append(_IRR_TEXT('\\'));
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
            //IAppLogger::logCritical("IUtility::createPath", "new path=%s", iPath.c_str());
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
//static array<stringc> getFilesInDirectory(stringc path) {
//    array<stringc> fileList;
//#ifdef APP_PLATFORM_WINDOWS
//    stringc search = path + "\\" + stringc("*.*");
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
} //namespace irr
