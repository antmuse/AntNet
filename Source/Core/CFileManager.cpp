#include "CFileManager.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef APP_PLATFORM_WINDOWS
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <shlwapi.h>
#include <tchar.h>
#endif

#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "coreutil.h"



namespace irr {
namespace io {

u32 CPathList::addNode(const fschar_t* fname, bool isDir) {
    SPathNode nd(fname);
    if (isDir) {
        nd.mID = mPaths.size();
        mPaths.push_back(nd);
        return mPaths.size();
    }
    nd.mID = mFiles.size();
    mFiles.push_back(nd);
    return mFiles.size();
}


///////////////////////////////////////////////////////////////////////////////////
CFileManager::CFileManager() :mWorkPath(getStartWorkPath()) {
}

CFileManager::CFileManager(const io::path& iVal) : mWorkPath(iVal) {
}

CFileManager::~CFileManager() {
}

const io::path& CFileManager::getStartWorkPath() {
    static io::path startWorkPath = getWorkPath();
    return startWorkPath;
}

bool CFileManager::createPath(const io::path& iPath) {
    bool ret = true;
#if defined(APP_PLATFORM_WINDOWS)
    io::path realpath;
    io::path directory(_IRR_TEXT(".\\"));
    core::splitFilename(iPath, &realpath);
    realpath.replace(_IRR_TEXT('\\'), _IRR_TEXT('/'));
    s32 start = 0;
    s32 end = realpath.size();
    while (start < end) {
        s32 pos = realpath.findNext(_IRR_TEXT('/'), start);
        pos = (pos > 0 ? pos : end - 1);
        directory += realpath.subString(start, pos - start);
        directory.append(_IRR_TEXT('\\'));
        start = pos + 1;
        if (::PathFileExists(directory.c_str())) {
            continue;
        }
        if (FALSE == ::CreateDirectory(directory.c_str(), 0)) {
            ret = false;
            break;
        }
    }
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    DIR * pDirect = opendir(iPath.c_str());
    if (0 == pDirect) {
        if (0 == ::mkdir(iPath.c_str(), 0777)) {
            //IAppLogger::logCritical("IUtility::createPath", "new path=%s", iPath.c_str());
            //CEngine::mPrinter->addW(L"created new path");
        } else {
            ret = false; //"path create error");
        }
    } else {
        ::closedir(pDirect);        //path already there
    }
#endif
    return ret;
}

#ifdef APP_PLATFORM_WINDOWS
bool AppLoadWindowsDrives(CPathList* iRet) {
    const u32 len = GetLogicalDriveStrings(0, nullptr);
    fschar_t buf[512];
    const bool created = len > 512;
    fschar_t* drives = created ? new fschar_t[len] : buf;
    if (!GetLogicalDriveStrings(len, drives)) {
        if (created) {
            delete[] drives;
        }
        return false;
    }
    fschar_t* temp = drives;
    for (fschar_t* drv = nullptr; *temp != 0; temp++) {
        drv = temp;
        while (*(++temp)) {
            if ('\\' == temp[0]) {
                temp[0] = 0;
            }
        };
        u32 typ = GetDriveType(drv);
        if (DRIVE_REMOVABLE == typ) {
            iRet->addNode(drv, true);
        } else if (DRIVE_FIXED == typ) {
            iRet->addNode(drv, true);
        }
    }
    if (created) {
        delete[] drives;
    }
    return true;
}
#endif //APP_PLATFORM_WINDOWS

CPathList* CFileManager::createFileList(bool readHide) {
    CPathList* ret = new CPathList(getCurrentPath());

#ifdef APP_PLATFORM_WINDOWS
    if (1 == getCurrentPath().size() && '/' == getCurrentPath()[0]) {
        AppLoadWindowsDrives(ret);
        return ret;
    }
    intptr_t hFile;
    struct _tfinddata_t c_file;
    if ((hFile = _tfindfirst(_T("*"), &c_file)) != -1L) {
        do {
            if (((_A_SYSTEM | _A_HIDDEN) & c_file.attrib) != 0 && !readHide) {
                continue;
            }
            ret->addNode(c_file.name, (_A_SUBDIR & c_file.attrib) != 0);
        } while (_tfindnext(hFile, &c_file) == 0);

        _findclose(hFile);
    }

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    //! We use the POSIX compliant methods instead of scandir
    DIR* dirHandle = opendir(getCurrentPath().c_str());
    if (dirHandle) {
        struct dirent* dirEntry;
        while ((dirEntry = readdir(dirHandle))) {
            u32 size = 0;
            bool isDirectory = false;
            if ((strcmp(dirEntry->d_name, ".") == 0) ||
                (strcmp(dirEntry->d_name, "..") == 0)) {
                continue;
            }
            struct stat buf;
            if (stat(dirEntry->d_name, &buf) == 0) {
                size = buf.st_size;
                isDirectory = S_ISDIR(buf.st_mode);
            } else {
                isDirectory = dirEntry->d_type == DT_DIR;
            }
            r->addNode(dirEntry->d_name, isDirectory);
        }
        closedir(dirHandle);
    }
#endif

    ret->sort();
    return ret;
}


bool CFileManager::existFile(const io::path& filename) const {
#if defined(_MSC_VER)
#if defined(_IRR_WCHAR_FILESYSTEM)
    return (_waccess(filename.c_str(), 0) != -1);
#else
    return (_access(filename.c_str(), 0) != -1);
#endif

#elif defined(F_OK)
#if defined(_IRR_WCHAR_FILESYSTEM)
    return (_waccess(filename.c_str(), F_OK) != -1);
#else
    return (access(filename.c_str(), F_OK) != -1);
#endif
#else
    return (access(filename.c_str(), 0) != -1);
#endif
}


io::path CFileManager::getFilePath(const io::path& filename) const {
    // find last forward or backslash
    s32 lastSlash = filename.findLast('/');
    const s32 lastBackSlash = filename.findLast('\\');
    lastSlash = lastSlash > lastBackSlash ? lastSlash : lastBackSlash;

    if ((u32)lastSlash < filename.size()) {
        return filename.subString(0, lastSlash);
    } else {
        return _IRR_TEXT(".");
    }
}


io::path CFileManager::getFileBasename(const io::path& filename, bool keepExtension) const {
    // find last forward or backslash
    s32 lastSlash = filename.findLast('/');
    const s32 lastBackSlash = filename.findLast('\\');
    lastSlash = core::max_(lastSlash, lastBackSlash);

    // get number of chars after last dot
    s32 end = 0;
    if (!keepExtension) {
        // take care to search only after last slash to check only for
        // dots in the filename
        end = filename.findLast('.');
        if (end == -1 || end < lastSlash)
            end = 0;
        else
            end = filename.size() - end;
    }

    if ((u32)lastSlash < filename.size())
        return filename.subString(lastSlash + 1, filename.size() - lastSlash - 1 - end);
    else if (end != 0)
        return filename.subString(0, filename.size() - end);
    else
        return filename;
}


io::path& CFileManager::flattenFilename(io::path& directory, const io::path& root) const {
    directory.replace('\\', '/');
    if (directory.lastChar() != '/') {
        directory.append('/');
    }
    io::path dir;
    io::path subdir;

    s32 lastpos = 0;
    s32 pos = 0;
    bool lastWasRealDir = false;

    while ((pos = directory.findNext('/', lastpos)) >= 0) {
        subdir = directory.subString(lastpos, pos - lastpos + 1);

        if (subdir == _IRR_TEXT("../")) {
            if (lastWasRealDir) {
                deletePathFromPath(dir, 2);
                lastWasRealDir = (dir.size() != 0);
            } else {
                dir.append(subdir);
                lastWasRealDir = false;
            }
        } else if (subdir == _IRR_TEXT("/")) {
            dir = root;
        } else if (subdir != _IRR_TEXT("./")) {
            dir.append(subdir);
            lastWasRealDir = true;
        }

        lastpos = pos + 1;
    }
    directory = dir;
    return directory;
}


path CFileManager::getRelativeFilename(const path& filename, const path& directory) const {
    if (filename.empty() || directory.empty()) {
        return filename;
    }
    io::path path1, file, ext;
    core::splitFilename(getAbsolutePath(filename), &path1, &file, &ext);
    io::path path2(getAbsolutePath(directory));
    core::list<io::path> list1, list2;
    path1.split(list1, _IRR_TEXT("/\\"), 2);
    path2.split(list2, _IRR_TEXT("/\\"), 2);
    u32 i = 0;
    core::list<io::path>::ConstIterator it1, it2;
    it1 = list1.begin();
    it2 = list2.begin();

#if defined (APP_PLATFORM_WINDOWS)
    fschar_t partition1 = 0, partition2 = 0;
    io::path prefix1, prefix2;
    if (it1 != list1.end())
        prefix1 = *it1;
    if (it2 != list2.end())
        prefix2 = *it2;
    if (prefix1.size() > 1 && prefix1[1] == _IRR_TEXT(':'))
        partition1 = core::locale_lower(prefix1[0]);
    if (prefix2.size() > 1 && prefix2[1] == _IRR_TEXT(':'))
        partition2 = core::locale_lower(prefix2[0]);

    // must have the same prefix or we can't resolve it to a relative filename
    if (partition1 != partition2) {
        return filename;
    }
#endif


    for (; i < list1.size() && i < list2.size()
#if defined (APP_PLATFORM_WINDOWS)
        && (io::path(*it1).make_lower() == io::path(*it2).make_lower())
#else
        && (*it1 == *it2)
#endif
        ; ++i) {
        ++it1;
        ++it2;
    }
    path1 = _IRR_TEXT("");
    for (; i < list2.size(); ++i) {
        path1 += _IRR_TEXT("../");
    }
    while (it1 != list1.end()) {
        path1 += *it1++;
        path1 += _IRR_TEXT('/');
    }
    path1 += file;
    if (ext.size()) {
        path1 += _IRR_TEXT('.');
        path1 += ext;
    }
    return path1;
}


io::path CFileManager::getWorkPath() {
    io::path wkpath;

#if defined(APP_PLATFORM_WINDOWS)
    fschar_t tmp[_MAX_PATH];
#if defined(_IRR_WCHAR_FILESYSTEM )
    _wgetcwd(tmp, _MAX_PATH);
#else
    _getcwd(tmp, _MAX_PATH);
#endif
    wkpath = tmp;
    wkpath.append(_IRR_TEXT('/'));
    wkpath.replace(_IRR_TEXT('\\'), _IRR_TEXT('/'));

#elif (defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID))
#if defined(_IRR_WCHAR_FILESYSTEM )
    u32 pathSize = 256;
    wchar_t* tmpPath = new wchar_t[pathSize];
    while ((pathSize < (1 << 16)) && !(wgetcwd(tmpPath, pathSize))) {
        delete[] tmpPath;
        pathSize *= 2;
        tmpPath = new char[pathSize];
    }
    if (tmpPath) {
        wkpath = tmpPath;
        delete[] tmpPath;
    }
#else
    u32 pathSize = 256;
    char* tmpPath = new char[pathSize];
    while ((pathSize < (1 << 16)) && !(getcwd(tmpPath, pathSize))) {
        delete[] tmpPath;
        pathSize *= 2;
        tmpPath = new char[pathSize];
    }
    if (tmpPath) {
        wkpath = tmpPath;
        delete[] tmpPath;
    }
#endif
#endif

    wkpath.validate();
    return wkpath;
}


bool CFileManager::setWorkPath(const io::path& newDirectory) {
    bool success = false;
    const fschar_t* wpth = newDirectory.c_str();

#if defined(_MSC_VER)
    if (newDirectory.size() > 1 && '/' == newDirectory[0]) {
        ++wpth;
    }
#if defined(_IRR_WCHAR_FILESYSTEM)
    success = (_wchdir(wpth) == 0);
#else
    success = (_chdir(wpth) == 0);
#endif
#else
#if defined(_IRR_WCHAR_FILESYSTEM)
    success = (_wchdir(wpth) == 0);
#else
    success = (chdir(wpth) == 0);
#endif
#endif
    return success;
}


io::path CFileManager::getAbsolutePath(const io::path& filename) const {
#if defined(APP_PLATFORM_WINDOWS)
    fschar_t* p = 0;
    fschar_t fpath[_MAX_PATH];
#if defined(_IRR_WCHAR_FILESYSTEM )
    p = _wfullpath(fpath, filename.c_str(), _MAX_PATH);
    core::stringw tmp(p);
    tmp.replace(L'\\', L'/');
#else
    p = _fullpath(fpath, filename.c_str(), _MAX_PATH);
    core::stringc tmp(p);
    tmp.replace('\\', '/');
#endif
    return tmp;
#elif (defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID))
    c8* p = 0;
    c8 fpath[4096];
    fpath[0] = 0;
    p = realpath(filename.c_str(), fpath);
    if (!p) {
        // content in fpath is unclear at this point
        if (!fpath[0]) // seems like fpath wasn't altered, use our best guess
        {
            io::path tmp(filename);
            io::path rot("");
            return flattenFilename(tmp, rot);
        } else
            return io::path(fpath);
    }
    if (filename[filename.size() - 1] == '/')
        return io::path(p) + _IRR_TEXT("/");
    else
        return io::path(p);
#else
    return io::path(filename);
#endif
}

} // end namespace io
} // end namespace irr

