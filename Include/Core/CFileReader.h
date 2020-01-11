#ifndef APP_CFILEREADER_H
#define	APP_CFILEREADER_H

#include "HConfig.h"
#include <stdarg.h>
#include "path.h"

namespace irr {
namespace io {


class CFileReader {
public:
    CFileReader();

    ~CFileReader();

    //! returns if file is open
    inline bool isOpen() const {
        return mFile != 0;
    }

    /** @return how much was read*/
    u64 read(void* buffer, u64 size);


    //! changes position in file, returns true if successful
    //! if relativeMovement==true, the pos is changed relative to current pos,
    //! otherwise from begin of file
    bool seek(s64 finalPos, bool relativeMovement);


    s64 getPos() const;


    bool openFile(const io::path& fileName);


    const io::path& getFileName() const {
        return mFilename;
    }

    s64 getFileSize()const {
        return mFileSize;
    }

    void close();

protected:
    io::path mFilename;
    FILE* mFile;
    s64 mFileSize;

private:
    CFileReader(const CFileReader&) = delete;
    CFileReader& operator=(const CFileReader&) = delete;
};


} // end namespace io
} // end namespace irr

#endif //APP_CFILEREADER_H

