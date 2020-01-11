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

u32 CPathList::addNode(const io::path& fullname, const io::path& fname, bool isDir) {
    SPathNode nd = {
        mNodes.size(),
        fullname,
        fname,
        isDir
    };
    mNodes.push_back(nd);
    return mNodes.size();
}


///////////////////////////////////////////////////////////////////////////////////
io::path CFileManager::GStartWorkPath = "";

CFileManager::CFileManager() {
    //fschar_t dest[260];
    //if (GetModuleFileName(0, dest, 260) > 0) {
    //}
    //setWorkPath(GStartWorkPath);
}

CFileManager::~CFileManager() {
}

void CFileManager::setStartWorkPath(const fschar_t* onpath) {
    if(onpath) {
        GStartWorkPath = onpath;
        GStartWorkPath.replace('\\', '/');
        //if (GStartWorkPath.lastChar() != '/') {
        //    GStartWorkPath.append('/');
        //}
    }
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
    DIR * pDirect = opendir(iPath.c_str());
    if(0 == pDirect) {
        if(0 == ::mkdir(iPath.c_str(), 0777)) {
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


CPathList* CFileManager::createFileList() {
    CPathList* r = 0;
    io::path Path = getWorkPath();
    Path.replace('\\', '/');
    if(Path.lastChar() != '/') {
        Path.append('/');
    }

#ifdef APP_PLATFORM_WINDOWS
    r = new CPathList(Path);
    intptr_t hFile;
    struct _tfinddata_t c_file;
    if((hFile = _tfindfirst(_T("*"), &c_file)) != -1L) {
        do {
            r->addNode(Path + c_file.name, c_file.name, (_A_SUBDIR & c_file.attrib) != 0);
        } while(_tfindnext(hFile, &c_file) == 0);

        _findclose(hFile);
    }

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    r = new CPathList(Path);

    //! We use the POSIX compliant methods instead of scandir
    DIR* dirHandle = opendir(Path.c_str());
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
            r->addNode(Path + dirEntry->d_name, dirEntry->d_name, isDirectory);
        }
        closedir(dirHandle);
    }
#endif

    r->sort();
    return r;
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

    if((u32)lastSlash < filename.size()) {
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


io::path& CFileManager::flattenFilename(io::path& directory, const io::path& root) const {
    directory.replace('\\', '/');
    if(directory.lastChar() != '/') {
        directory.append('/');
    }
    io::path dir;
    io::path subdir;

    s32 lastpos = 0;
    s32 pos = 0;
    bool lastWasRealDir = false;

    while((pos = directory.findNext('/', lastpos)) >= 0) {
        subdir = directory.subString(lastpos, pos - lastpos + 1);

        if(subdir == _IRR_TEXT("../")) {
            if(lastWasRealDir) {
                deletePathFromPath(dir, 2);
                lastWasRealDir = (dir.size() != 0);
            } else {
                dir.append(subdir);
                lastWasRealDir = false;
            }
        } else if(subdir == _IRR_TEXT("/")) {
            dir = root;
        } else if(subdir != _IRR_TEXT("./")) {
            dir.append(subdir);
            lastWasRealDir = true;
        }

        lastpos = pos + 1;
    }
    directory = dir;
    return directory;
}


path CFileManager::getRelativeFilename(const path& filename, const path& directory) const {
    if(filename.empty() || directory.empty()) {
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
    if(it1 != list1.end())
        prefix1 = *it1;
    if(it2 != list2.end())
        prefix2 = *it2;
    if(prefix1.size() > 1 && prefix1[1] == _IRR_TEXT(':'))
        partition1 = core::locale_lower(prefix1[0]);
    if(prefix2.size() > 1 && prefix2[1] == _IRR_TEXT(':'))
        partition2 = core::locale_lower(prefix2[0]);

    // must have the same prefix or we can't resolve it to a relative filename
    if(partition1 != partition2) {
        return filename;
    }
#endif


    for(; i < list1.size() && i < list2.size()
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
    for(; i < list2.size(); ++i) {
        path1 += _IRR_TEXT("../");
    }
    while(it1 != list1.end()) {
        path1 += *it1++;
        path1 += _IRR_TEXT('/');
    }
    path1 += file;
    if(ext.size()) {
        path1 += _IRR_TEXT('.');
        path1 += ext;
    }
    return path1;
}


const io::path& CFileManager::getWorkPath() {
#if defined(APP_PLATFORM_WINDOWS)
    fschar_t tmp[_MAX_PATH];
#if defined(_IRR_WCHAR_FILESYSTEM )
    _wgetcwd(tmp, _MAX_PATH);
    mWorkPath = tmp;
    mWorkPath.replace(L'\\', L'/');
#else
    _getcwd(tmp, _MAX_PATH);
    mWorkPath = tmp;
    mWorkPath.replace('\\', '/');
#endif

#elif (defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID))
#if defined(_IRR_WCHAR_FILESYSTEM )
    u32 pathSize = 256;
    wchar_t* tmpPath = new wchar_t[pathSize];
    while((pathSize < (1 << 16)) && !(wgetcwd(tmpPath, pathSize))) {
        delete[] tmpPath;
        pathSize *= 2;
        tmpPath = new char[pathSize];
    }
    if(tmpPath) {
        mWorkPath = tmpPath;
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
        mWorkPath = tmpPath;
        delete[] tmpPath;
    }
#endif
#endif

    mWorkPath.validate();
    return mWorkPath;
}


bool CFileManager::setWorkPath(const io::path& newDirectory) {
    bool success = false;

#if defined(_MSC_VER)
#if defined(_IRR_WCHAR_FILESYSTEM)
    success = (_wchdir(newDirectory.c_str()) == 0);
#else
    success = (_chdir(newDirectory.c_str()) == 0);
#endif
#else
#if defined(_IRR_WCHAR_FILESYSTEM)
    success = (_wchdir(newDirectory.c_str()) == 0);
#else
    success = (chdir(newDirectory.c_str()) == 0);
#endif
#endif
    if(success) {
        mWorkPath = newDirectory;
    }
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
    if(!p) {
        // content in fpath is unclear at this point
        if(!fpath[0]) // seems like fpath wasn't altered, use our best guess
        {
            io::path tmp(filename);
            io::path rot("");
            return flattenFilename(tmp, rot);
        } else
            return io::path(fpath);
    }
    if(filename[filename.size() - 1] == '/')
        return io::path(p) + _IRR_TEXT("/");
    else
        return io::path(p);
#else
    return io::path(filename);
#endif
}

} // end namespace io
} // end namespace irr
