#ifndef APP_CFILEWRITER_H
#define	APP_CFILEWRITER_H

#include "HConfig.h"
#include <stdarg.h>
#include "path.h"

namespace irr {
namespace io {


class CFileWriter {
public:
    CFileWriter();

    ~CFileWriter();

    //! returns if file is open
    inline bool isOpen() const {
        return mFile != 0;
    }

    bool flush();

    //! returns how much was read
    u64 write(const void* buffer, u64 sizeToWrite);

    u64 writeParams(const c8* format, ...);
    u64 writeParams(const c8* format, va_list& param);
    u64 writeWParams(const wchar_t* format, ...);
    u64 writeWParams(const wchar_t* format, va_list& param);


    //! changes position in file, returns true if successful
    //! if relativeMovement==true, the pos is changed relative to current pos,
    //! otherwise from begin of file
    bool seek(s64 finalPos, bool relativeMovement);


    s64 getPos() const;


    void openFile(const io::path& fileName, bool append);


    const io::path& getFileName() const {
        return mFilename;
    }

    s64 getFileSize()const {
        return mFileSize;
    }

protected:
    io::path mFilename;
    FILE* mFile;
    s64 mFileSize;
};


} // end namespace io
} // end namespace irr

#endif //APP_CFILEWRITER_H

