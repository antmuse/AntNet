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


namespace app {
namespace io {

u32 CPathList::addNode(const tchar* fname, bool isDir) {
    SPathNode nd(fname);
    if(isDir) {
        nd.mID = mPaths.size();
        mPaths.pushBack(nd);
        return mPaths.size();
    }
    nd.mID = mFiles.size();
    mFiles.pushBack(nd);
    return mFiles.size();
}


///////////////////////////////////////////////////////////////////////////////////
CFileManager::CFileManager() :mWorkPath(getStartWorkPath()) {
}

CFileManager::CFileManager(const core::CPath& iVal) : mWorkPath(iVal) {
}

CFileManager::~CFileManager() {
}

const core::CPath& CFileManager::getStartWorkPath() {
    static core::CPath startWorkPath = getWorkPath();
    return startWorkPath;
}

bool CFileManager::createPath(const core::CPath& iPath) {
    bool ret = true;
#if defined(APP_PLATFORM_WINDOWS)
    core::CPath realpath;
    core::CPath directory(APP_STR(".\\"));
    iPath.splitFilename(&realpath);
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
    DIR* pDirect = opendir(iPath.c_str());
    if(0 == pDirect) {
        if(0 == ::mkdir(iPath.c_str(), 0777)) {
            //CLogger::logCritical("IUtility::createPath", "new path=%s", iPath.c_str());
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
    tchar buf[512];
    const bool created = len > 512;
    tchar* drives = created ? new tchar[len] : buf;
    if(!GetLogicalDriveStrings(len, drives)) {
        if(created) {
            delete[] drives;
        }
        return false;
    }
    tchar* temp = drives;
    for(tchar* drv = nullptr; *temp != 0; temp++) {
        drv = temp;
        while(*(++temp)) {
            if('\\' == temp[0]) {
                temp[0] = 0;
            }
        };
        u32 typ = GetDriveType(drv);
        if(DRIVE_REMOVABLE == typ) {
            iRet->addNode(drv, true);
        } else if(DRIVE_FIXED == typ) {
            iRet->addNode(drv, true);
        }
    }
    if(created) {
        delete[] drives;
    }
    return true;
}
#endif //APP_PLATFORM_WINDOWS

CPathList* CFileManager::createFileList(bool readHide) {
    CPathList* ret = new CPathList(getCurrentPath());

#ifdef APP_PLATFORM_WINDOWS
    if(1 == getCurrentPath().size() && '/' == getCurrentPath()[0]) {
        AppLoadWindowsDrives(ret);
        return ret;
    }
    intptr_t hFile;
    struct _tfinddata_t c_file;
    if((hFile = _tfindfirst(_T("*"), &c_file)) != -1L) {
        do {
            if(((_A_SYSTEM | _A_HIDDEN) & c_file.attrib) != 0 && !readHide) {
                continue;
            }
            if((_tcscmp(c_file.name, APP_STR(".")) == 0) ||
                (_tcscmp(c_file.name, APP_STR("..")) == 0)) {
                continue;
            }
            ret->addNode(c_file.name, (_A_SUBDIR & c_file.attrib) != 0);
        } while(_tfindnext(hFile, &c_file) == 0);

        _findclose(hFile);
    }

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    //! We use the POSIX compliant methods instead of scandir
    DIR* dirHandle = opendir(getCurrentPath().c_str());
    if(dirHandle) {
        struct dirent* dirEntry;
        while((dirEntry = readdir(dirHandle))) {
            u32 size = 0;
            bool isDirectory = false;
            if((strcmp(dirEntry->d_name, ".") == 0) ||
                (strcmp(dirEntry->d_name, "..") == 0)) {
                continue;
            }
            struct stat buf;
            if(stat(dirEntry->d_name, &buf) == 0) {
                size = buf.st_size;
                isDirectory = S_ISDIR(buf.st_mode);
            } else {
                isDirectory = dirEntry->d_type == DT_DIR;
            }
            ret->addNode(dirEntry->d_name, isDirectory);
        }
        closedir(dirHandle);
    }
#endif

    ret->sort();
    return ret;
}


bool CFileManager::existFile(const core::CPath& filename) {
#if defined(_MSC_VER)
#if defined(APP_WCHAR_SYS)
    return (_waccess(filename.c_str(), 0) != -1);
#else
    return (_access(filename.c_str(), 0) != -1);
#endif

#elif defined(F_OK)
#if defined(APP_WCHAR_SYS)
    return (_waccess(filename.c_str(), F_OK) != -1);
#else
    return (access(filename.c_str(), F_OK) != -1);
#endif
#else
    return (access(filename.c_str(), 0) != -1);
#endif
}

bool CFileManager::deleteFile(const core::CPath& filename) {
#if defined(APP_PLATFORM_WINDOWS)
    return TRUE == DeleteFile(filename.c_str());
#else
    return 0 == remove(filename.c_str());
#endif
}

core::CPath CFileManager::getFilePath(const core::CPath& filename) const {
    // find last forward or backslash
    s32 lastSlash = filename.findLast('/');
    const s32 lastBackSlash = filename.findLast('\\');
    lastSlash = lastSlash > lastBackSlash ? lastSlash : lastBackSlash;

    if((u32)lastSlash < filename.size()) {
        return filename.subString(0, lastSlash);
    } else {
        return APP_STR(".");
    }
}


core::CPath CFileManager::getFileBasename(const core::CPath& filename, bool keepExtension) const {
    // find last forward or backslash
    s32 lastSlash = filename.findLast('/');
    const s32 lastBackSlash = filename.findLast('\\');
    lastSlash = core::max_(lastSlash, lastBackSlash);

    // get number of chars after last dot
    s32 end = 0;
    if(!keepExtension) {
        // take care to search only after last slash to check only for
        // dots in the filename
        end = filename.findLast('.');
        if(end == -1 || end < lastSlash)
            end = 0;
        else
            end = filename.size() - end;
    }

    if((u32)lastSlash < filename.size())
        return filename.subString(lastSlash + 1, filename.size() - lastSlash - 1 - end);
    else if(end != 0)
        return filename.subString(0, filename.size() - end);
    else
        return filename;
}


core::CPath& CFileManager::flattenFilename(core::CPath& directory, const core::CPath& root) const {
    directory.replace('\\', '/');
    if(directory.lastChar() != '/') {
        directory.append('/');
    }
    core::CPath dir;
    core::CPath subdir;

    s32 lastpos = 0;
    s32 pos = 0;
    bool lastWasRealDir = false;

    while((pos = directory.findNext('/', lastpos)) >= 0) {
        subdir = directory.subString(lastpos, pos - lastpos + 1);

        if(subdir == APP_STR("../")) {
            if(lastWasRealDir) {
                dir.deletePathFromPath(2);
                lastWasRealDir = (dir.size() != 0);
            } else {
                dir.append(subdir);
                lastWasRealDir = false;
            }
        } else if(subdir == APP_STR("/")) {
            dir = root;
        } else if(subdir != APP_STR("./")) {
            dir.append(subdir);
            lastWasRealDir = true;
        }

        lastpos = pos + 1;
    }
    directory = dir;
    return directory;
}


core::CPath CFileManager::getRelativeFilename(const core::CPath& filename, const core::CPath& directory) const {
    if(filename.empty() || directory.empty()) {
        return filename;
    }
    core::CPath path1, file, ext;
    getAbsolutePath(filename).splitFilename(&path1, &file, &ext);
    core::CPath path2(getAbsolutePath(directory));
    core::TList<core::CPath> list1, list2;
    path1.split(list1, APP_STR("/\\"), 2);
    path2.split(list2, APP_STR("/\\"), 2);
    u32 i = 0;
    core::TList<core::CPath>::ConstIterator it1, it2;
    it1 = list1.begin();
    it2 = list2.begin();

#if defined (APP_PLATFORM_WINDOWS)
    tchar partition1 = 0, partition2 = 0;
    core::CPath prefix1, prefix2;
    if(it1 != list1.end())
        prefix1 = *it1;
    if(it2 != list2.end())
        prefix2 = *it2;
    if(prefix1.size() > 1 && prefix1[1] == APP_STR(':'))
        partition1 = core::App2Lower(prefix1[0]);
    if(prefix2.size() > 1 && prefix2[1] == APP_STR(':'))
        partition2 = core::App2Lower(prefix2[0]);

    // must have the same prefix or we can't resolve it to a relative filename
    if(partition1 != partition2) {
        return filename;
    }
#endif


    for(; i < list1.size() && i < list2.size()
#if defined (APP_PLATFORM_WINDOWS)
        && (core::CPath(*it1).makeLower() == core::CPath(*it2).makeLower())
#else
        && (*it1 == *it2)
#endif
        ; ++i) {
        ++it1;
        ++it2;
    }
    path1 = APP_STR("");
    for(; i < list2.size(); ++i) {
        path1 += APP_STR("../");
    }
    while(it1 != list1.end()) {
        path1 += *it1++;
        path1 += APP_STR('/');
    }
    path1 += file;
    if(ext.size()) {
        path1 += APP_STR('.');
        path1 += ext;
    }
    return path1;
}


core::CPath CFileManager::getWorkPath() {
    core::CPath wkpath;

#if defined(APP_PLATFORM_WINDOWS)
    tchar tmp[_MAX_PATH];
#if defined(APP_WCHAR_SYS )
    _wgetcwd(tmp, _MAX_PATH);
#else
    _getcwd(tmp, _MAX_PATH);
#endif
    wkpath = tmp;
    wkpath.append(APP_STR('/'));
    wkpath.replace(APP_STR('\\'), APP_STR('/'));

#elif (defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID))
#if defined(APP_WCHAR_SYS )
    u32 pathSize = 256;
    wchar_t* tmpPath = new wchar_t[pathSize];
    while((pathSize < (1 << 16)) && !(wgetcwd(tmpPath, pathSize))) {
        delete[] tmpPath;
        pathSize *= 2;
        tmpPath = new char[pathSize];
    }
    if(tmpPath) {
        wkpath = tmpPath;
        delete[] tmpPath;
    }
#else
    u32 pathSize = 256;
    char* tmpPath = new char[pathSize];
    while((pathSize < (1 << 16)) && !(getcwd(tmpPath, pathSize))) {
        delete[] tmpPath;
        pathSize *= 2;
        tmpPath = new char[pathSize];
    }
    if(tmpPath) {
        wkpath = tmpPath;
        delete[] tmpPath;
    }
#endif
#endif

    wkpath.validate();
    return wkpath;
}


bool CFileManager::setWorkPath(const core::CPath& newDirectory) {
    bool success = false;
    const tchar* wpth = newDirectory.c_str();

#if defined(_MSC_VER)
    if(newDirectory.size() > 1 && '/' == newDirectory[0]) {
        ++wpth;
    }
#if defined(APP_WCHAR_SYS)
    success = (_wchdir(wpth) == 0);
#else
    success = (_chdir(wpth) == 0);
#endif
#else
#if defined(APP_WCHAR_SYS)
    success = (_wchdir(wpth) == 0);
#else
    success = (chdir(wpth) == 0);
#endif
#endif
    return success;
}


core::CPath CFileManager::getAbsolutePath(const core::CPath& filename) const {
#if defined(APP_PLATFORM_WINDOWS)
    tchar* p = 0;
    tchar fpath[_MAX_PATH];
#if defined(APP_WCHAR_SYS)
    p = _wfullpath(fpath, filename.c_str(), _MAX_PATH);
#else
    p = _fullpath(fpath, filename.c_str(), _MAX_PATH);
#endif
    core::CPath tmp(p);
    tmp.replace('\\', '/');
    return tmp;
#elif (defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID))
    s8* p = 0;
    s8 fpath[4096];
    fpath[0] = 0;
    p = realpath(filename.c_str(), fpath);
    if(!p) {
        // content in fpath is unclear at this point
        if(!fpath[0]) // seems like fpath wasn't altered, use our best guess
        {
            core::CPath tmp(filename);
            core::CPath rot("");
            return flattenFilename(tmp, rot);
        } else
            return core::CPath(fpath);
    }
    if(filename[filename.size() - 1] == '/')
        return core::CPath(p) + APP_STR("/");
    else
        return core::CPath(p);
#else
    return core::CPath(filename);
#endif
}

} // end namespace io
} // end namespace app

