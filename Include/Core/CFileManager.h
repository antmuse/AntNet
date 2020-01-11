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

    //! The name of the file
    io::path mName;

    //! The name of the file including the path
    io::path mFullName;


    //! True if this is a folder, false if not.
    bool mDirectory;

    //! The == operator is provided so that CFileList can slowly search the list!
    bool operator==(const struct SPathNode& other) const {
        if (mDirectory != other.mDirectory) {
            return false;
        }
        return mFullName.equals_ignore_case(other.mFullName);
    }

    //! The < operator is provided so that CFileList can sort and quickly search the list.
    bool operator<(const struct SPathNode& other) const {
        if (mDirectory != other.mDirectory) {
            return mDirectory;
        }
        return mFullName.lower_ignore_case(other.mFullName);
    }
};

class CPathList {
public:
    CPathList(const io::path& workPath) {
    }
    ~CPathList() {
    }

    u32 getSize()const {
        return mNodes.size();
    }

    void sort() {
        mNodes.sort();
    }

    u32 addNode(const io::path& fullname, const io::path& fname, bool isDir);

private:
    io::path mWorkPath;
    core::array<SPathNode> mNodes;
};

class CFileManager {
public:
    static io::path GStartWorkPath;
    static void setStartWorkPath(const fschar_t* path);

    CFileManager();

    ~CFileManager();

    bool resetWorkPath() {
        return setWorkPath(GStartWorkPath);
    }

    //! flatten a path and file name for example: "/you/me/../." becomes "/you"
    io::path& flattenFilename(io::path& directory, const io::path& root) const;

    //! Get the relative filename, relative to the given directory
    io::path getRelativeFilename(const path& filename, const path& directory) const;

    //! Returns the string of the current working directory
    const io::path& getWorkPath();

    //! Changes the current Working Directory to the given string.
    bool setWorkPath(const io::path& newDirectory);

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

    //! Creates a list of files and directories in the current working directory
    CPathList* createFileList();

    bool createPath(const io::path& iPath);

    const io::path& getCurrentPath() const {
        return mWorkPath;
    }

protected:
    io::path mWorkPath;
};


} // end namespace io
} // end namespace irr

#endif //APP_CFILEMANAGER_H

