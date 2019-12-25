#include "CFileWriter.h"
#include <stdio.h>

namespace irr {
namespace io {


CFileWriter::CFileWriter() :
    mFile(nullptr),
    mFileSize(0) {
}

CFileWriter::~CFileWriter() {
    if(mFile) {
        fclose(mFile);
        mFile = nullptr;
    }
}

bool CFileWriter::flush() {
    if(!isOpen()) {
        return false;
    }
    return 0 == fflush(mFile);
}


u64 CFileWriter::write(const void* buffer, u64 sizeToWrite) {
    if(!isOpen()) {
        return 0;
    }
    u64 ret = fwrite(buffer, 1, sizeToWrite, mFile);
    mFileSize += ret;
    return ret;
}

u64 CFileWriter::writeParams(const c8* format, va_list& args) {
    if(!isOpen()) {
        return 0;
    }
    u64 ret = vfprintf(mFile, format, args);
    mFileSize += ret;
    return ret;
}

u64 CFileWriter::writeWParams(const wchar_t* format, va_list& args) {
    if(!isOpen()) {
        return 0;
    }
    u64 ret = vfwprintf(mFile, format, args);
    mFileSize += ret;
    return ret;
}

u64 CFileWriter::writeParams(const c8* format, ...) {
    if(!isOpen()) {
        return 0;
    }
    va_list args;
    va_start(args, format);
    u64 ret = vfprintf(mFile, format, args);
    va_end(args);
    mFileSize += ret;
    return ret;
}

u64 CFileWriter::writeWParams(const wchar_t* format, ...) {
    if(!isOpen()) {
        return 0;
    }
    va_list args;
    va_start(args, format);
    u64 ret = vfwprintf(mFile, format, args);
    va_end(args);
    mFileSize += ret;
    return ret;
}


bool CFileWriter::seek(s64 finalPos, bool relativeMovement) {
    if(!isOpen()) {
        return false;
    }
#if defined(APP_PLATFORM_WINDOWS)
    return _fseeki64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return fseeko64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#endif
}

s64 CFileWriter::getPos() const {
#if defined(APP_PLATFORM_WINDOWS)
    return _ftelli64(mFile);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return ftello64(mFile);
#endif
}

void CFileWriter::openFile(const io::path& fileName, bool append) {
    if(fileName.size() == 0) {
        mFile = nullptr;
        return;
    }
    mFilename = fileName;
#if defined(_IRR_WCHAR_FILESYSTEM)
    mFile = _wfopen(mFilename.c_str(), append ? L"ab" : L"wb");
#else
    mFile = fopen(mFilename.c_str(), append ? "ab" : "wb");
#endif

    if(mFile) {
#if defined(APP_PLATFORM_WINDOWS)
        _fseeki64(mFile, 0, SEEK_END);
        mFileSize = _ftelli64(mFile);
        _fseeki64(mFile, 0, SEEK_SET);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
        fseeko64(mFile, 0, SEEK_END);
        mFileSize = ftello64(mFile);
        fseeko64(mFile, 0, SEEK_SET);
#endif

    }
}


} // end namespace io
} // end namespace irr

