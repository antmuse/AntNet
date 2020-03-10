#ifndef APP_CFILEMANAGER_H
#define	APP_CFILEMANAGER_H

#include "HConfig.h"
#include "path.h"
#include "irrList.h"
#include "irrArray.h"

namespace irr {
namespace io {

//! An entry in a list of files, can be a folder or a file.
struct SPathNode {
    u32 mID;
    io::path mName; //file or path
    SPathNode(const fschar_t* nam) : mID(0), mName(nam) {
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
    CPathList(const io::path& workPath) :mWorkPath(workPath) {
    }
    ~CPathList() {
    }
    const io::path& getWorkPath()const {
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
    u32 addNode(const fschar_t* fname, bool isDir);

private:
    io::path mWorkPath;
    core::array<SPathNode> mPaths;
    core::array<SPathNode> mFiles;
};

class CFileManager {
public:
    static const io::path& getStartWorkPath();

    //! Returns the string of the current working directory
    static io::path getWorkPath();

    //! Changes the current Working Directory to the given string.
    static bool setWorkPath(const io::path& newDirectory);


    CFileManager();

    CFileManager(const io::path& iVal);

    ~CFileManager();

    bool resetWorkPath() {
        return setWorkPath(getStartWorkPath());
    }

    //! flatten a path and file name for example: "/you/me/../." becomes "/you"
    io::path& flattenFilename(io::path& directory, const io::path& root) const;

    //! Get the relative filename, relative to the given directory
    io::path getRelativeFilename(const path& filename, const path& directory) const;

    io::path getAbsolutePath(const io::path& filename) const;

    /**
    * @return the base part of a filename(removed the directory part).
    *  If no directory path is prefixed, the full name is returned.
    */
    io::path getFileBasename(const io::path& filename, bool keepExtension = true) const;

    //! returns the directory part of a filename, i.e. all until the first
    //! slash or backslash, excluding it. If no directory path is prefixed, a '.'
    //! is returned.
    io::path getFilePath(const io::path& filename) const;

    //! determines if a file exists and would be able to be opened.
    bool existFile(const io::path& filename) const;

    /**
    * @brief Creates a list of files and directories in the current working directory
    * @param readHide got hide files and paths if true.
    */
    CPathList* createFileList(bool readHide = false);

    bool createPath(const io::path& iPath);

    void setCurrentPath(const io::path& iVal) {
        mWorkPath = iVal;
    }

    const io::path& getCurrentPath() const {
        return mWorkPath;
    }

protected:
    io::path mWorkPath;
};


} // end namespace io
} // end namespace irr

#endif //APP_CFILEMANAGER_H

