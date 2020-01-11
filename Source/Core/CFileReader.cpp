#include "CFileReader.h"
#include <stdio.h>

namespace irr {
namespace io {


CFileReader::CFileReader() :
    mFile(nullptr),
    mFileSize(0) {
}

CFileReader::~CFileReader() {
    close();
}

void CFileReader::close() {
    if(mFile) {
        fclose(mFile);
        mFile = nullptr;
    }
}


u64 CFileReader::read(void* buffer, u64 iSize) {
    if(!isOpen() || nullptr == buffer) {
        return 0;
    }
    return fread(buffer, 1, iSize, mFile);
}


bool CFileReader::seek(s64 finalPos, bool relativeMovement) {
    if(!isOpen()) {
        return false;
    }
#if defined(APP_PLATFORM_WINDOWS)
    return _fseeki64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return fseeko64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#endif
}

s64 CFileReader::getPos() const {
#if defined(APP_PLATFORM_WINDOWS)
    return _ftelli64(mFile);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    return ftello64(mFile);
#endif
}

bool CFileReader::openFile(const io::path& fileName) {
    if(fileName.empty()) {
        return false;
    }
    close();

    mFilename = fileName;
#if defined(_IRR_WCHAR_FILESYSTEM)
    mFile = _wfopen(mFilename.c_str(), L"rb");
#else
    mFile = fopen(mFilename.c_str(), "rb");
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
    return nullptr != mFile;
}


} // end namespace io
} // end namespace irr

