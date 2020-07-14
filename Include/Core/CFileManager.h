#ifndef APP_CFILEMANAGER_H
#define	APP_CFILEMANAGER_H

#include "HConfig.h"
#include "CString.h"
#include "AppList.h"
#include "AppArray.h"

namespace app {
namespace io {

//! An entry in a list of files, can be a folder or a file.
struct SPathNode {
    u32 mID;
    core::CPath mName; //file or path
    SPathNode(const tchar* nam) : mID(0), mName(nam) {
    }
    bool operator==(const struct SPathNode& other) const {
        return mName.equals_ignore_case(other.mName);
    }

    bool operator<(const struct SPathNode& other) const {
        return mName.lower_ignore_case(other.mName);
    }
};

class CPathList {
public:
    CPathList(const core::CPath& workPath) :mWorkPath(workPath) {
    }
    ~CPathList() {
    }
    const core::CPath& getWorkPath()const {
        return mWorkPath;
    }
    u32 getCount()const {
        return mPaths.size() + mFiles.size();
    }
    u32 getPathCount()const {
        return mPaths.size();
    }
    u32 getFileCount()const {
        return mFiles.size();
    }
    void sort() {
        mPaths.sort();
        mFiles.sort();
    }
    const SPathNode& getPath(u32 idx)const {
        return mPaths[idx];
    }
    const SPathNode& getFile(u32 idx)const {
        return mFiles[idx];
    }
    u32 addNode(const tchar* fname, bool isDir);

private:
    core::CPath mWorkPath;
    core::TArray<SPathNode> mPaths;
    core::TArray<SPathNode> mFiles;
};

class CFileManager {
public:
    static const core::CPath& getStartWorkPath();

    //! Returns the string of the current working directory
    static core::CPath getWorkPath();

    //! Changes the current Working Directory to the given string.
    static bool setWorkPath(const core::CPath& newDirectory);


    CFileManager();

    CFileManager(const core::CPath& iVal);

    ~CFileManager();

    bool resetWorkPath() {
        return setWorkPath(getStartWorkPath());
    }

    //! flatten a path and file name for example: "/you/me/../." becomes "/you"
    core::CPath& flattenFilename(core::CPath& directory, const core::CPath& root) const;

    //! Get the relative filename, relative to the given directory
    core::CPath getRelativeFilename(const core::CPath& filename, const core::CPath& directory) const;

    core::CPath getAbsolutePath(const core::CPath& filename) const;

    /**
    * @return the base part of a filename(removed the directory part).
    *  If no directory path is prefixed, the full name is returned.
    */
    core::CPath getFileBasename(const core::CPath& filename, bool keepExtension = true) const;

    //! returns the directory part of a filename, i.e. all until the first
    //! slash or backslash, excluding it. If no directory path is prefixed, a '.'
    //! is returned.
    core::CPath getFilePath(const core::CPath& filename) const;

    //! determines if a file exists and would be able to be opened.
    static bool existFile(const core::CPath& filename);

    static bool deleteFile(const core::CPath& filename);

    /**
    * @brief Creates a list of files and directories in the current working directory
    * @param readHide got hide files and paths if true.
    */
    CPathList* createFileList(bool readHide = false);

    static bool createPath(const core::CPath& iPath);

    void setCurrentPath(const core::CPath& iVal) {
        mWorkPath = iVal;
    }

    const core::CPath& getCurrentPath() const {
        return mWorkPath;
    }

protected:
    core::CPath mWorkPath;
};


} // end namespace io
} // end namespace app

#endif //APP_CFILEMANAGER_H

