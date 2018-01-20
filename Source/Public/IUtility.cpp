#include "IUtility.h"
#include "irrMath.h"
#include "fast_atof.h"
#include "coreutil.h"


#if defined(APP_PLATFORM_WINDOWS) && defined(_MSC_VER) && (_MSC_VER > 1298)
//#include <stdio.h>
//#include <stdlib.h>
#define bswap_16(X) _byteswap_ushort(X)
#define bswap_32(X) _byteswap_ulong(X)
#define bswap_64(X) _byteswap_uint64(X)
#elif defined(APP_PLATFORM_LINUX)
#include <byteswap.h>
#else
#define bswap_16(X) ((((X)&0xFF) << 8) | (((X)&0xFF00) >> 8))
#define bswap_32(X) ((((X)&0x000000FF)<<24) | (((X)&0xFF000000) >> 24) | (((X)&0x0000FF00) << 8) | (((X) &0x00FF0000) >> 8))
#define bswap_64(X) ( (((X)&0x00000000000000FF)<<56) | (((X)&0xFF00000000000000)>>56)  \
                    | (((X)&0x000000000000FF00)<<40) | (((X)&0x00FF000000000000)>>40)  \
                    | (((X)&0x0000000000FF0000)<<24) | (((X)&0x0000FF0000000000)>>24)  \
                    | (((X)&0x00000000FF000000)<< 8) | (((X)&0x000000FF00000000)>> 8))
#endif


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


IUtility& IUtility::getInstance() {
    static IUtility it;
    return it;
}


IUtility::IUtility() {
}


IUtility::~IUtility() {
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
    u32 tmp = IR(num);
    tmp = bswap_32(tmp);
    return (FR(tmp));
}


s64 IUtility::swapByte(s64 num) {
    return bswap_64(num);
}

u64 IUtility::swapByte(u64 num) {
    return bswap_64(num);
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


u32 IUtility::convertToHexString(const u8* iData, u32 iDataSize, c8* iResult, u32 iSize) {
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


u32 IUtility::convertToU8(const c8* iData, u32 iDataSize, u8* iResult, u32 iSize) {
    if(iDataSize > iSize * 2) {
        iDataSize = iSize * 2;
    }
    for(u32 i = 0; i < iDataSize; i += 2) {
        u8 a = convertToU8(*iData++);
        u8 b = convertToU8(*iData++);
        *iResult++ = a << 4 | b;
    }
    return iDataSize / 2;
}


void IUtility::print(const u8* iData, u32 iSize) {
    for(u32 i = 0; i < iSize; ++i) {
        printf("%02X", iData[i]);
    }
}


const c8* IUtility::skipFlag(const c8* iStart, const c8* const iEnd, const c8 iLeft, const c8 iRight) {
    iStart = goNextFlag(iStart, iEnd, iLeft);
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


const c8* IUtility::goNextFlag(const c8* iStart, const c8* const iEnd, const c8 ichar) {
    while((iStart < iEnd) && (*iStart != ichar)) {
        ++iStart;
    }
    return iStart;
}


const c8* IUtility::goBackFlag(const c8* iStart, const c8* iEnd, const c8 it) {
    while((iStart > iEnd) && (*iStart != it)) {
        --iStart;
    }
    return iStart;
}


const c8* IUtility::goNextLine(const c8* iStart, const c8* const iEnd) {
    while(iStart < iEnd) {
        if(*iStart == '\n') {
            ++iStart;
            break;
        }
        ++iStart;
    }
    return iStart;
}


const c8* IUtility::goAndCopyNextWord(c8* iOutBuffer, const c8* iStart, u32 outBufLength, const c8* iEnd, bool acrossNewlines) {
    iStart = goNextWord(iStart, iEnd, acrossNewlines);
    return iStart += copyWord(iOutBuffer, iStart, outBufLength, iEnd);
}


u32 IUtility::copyWord(c8* iOutBuffer, const c8* const iStart, u32 outBufLength, const c8* const iEnd) {
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


const c8* IUtility::goFirstWord(const c8* iStart, const c8* const iEnd, bool acrossNewlines) {
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


const c8* IUtility::goNextWord(const c8* iStart, const c8* const iEnd, bool acrossNewlines) {
    while((iStart < iEnd) && !core::isspace(*iStart)) {
        ++iStart;
    }
    return goFirstWord(iStart, iEnd, acrossNewlines);
}



bool IUtility::createPath(const io::path& iPath) {
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
    DIR* pDirect = opendir(iPath.c_str());
    if(0 == pDirect) {
        if(0 == mkdir(iPath.c_str(), 0777)) {
            //IAppLogger::logCritical("IUtility::createPath", "new path=%s", iPath.c_str());
            //CEngine::mPrinter->addW(L"created new path");
        } else {
            ret = false;
            //CEngine::mPrinter->addW(L"path create error");
        }
    } else {
        closedir(pDirect);
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



} //namespace irr
