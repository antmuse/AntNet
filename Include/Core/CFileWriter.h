#ifndef APP_CFILEWRITER_H
#define	APP_CFILEWRITER_H

#include "HConfig.h"
#include <stdarg.h>
#include "CString.h"

namespace app {
namespace io {


class CFileWriter {
public:
    CFileWriter();

    ~CFileWriter();

    void close();

    //! returns if file is open
    inline bool isOpen() const {
        return mFile != 0;
    }

    bool flush();

    //! returns how much was read
    u64 write(const void* buffer, u64 sizeToWrite);

    u64 writeParams(const s8* format, ...);
    u64 writeParams(const s8* format, va_list& param);
    u64 writeWParams(const wchar_t* format, ...);
    u64 writeWParams(const wchar_t* format, va_list& param);


    //! changes position in file, returns true if successful
    //! if relativeMovement==true, the pos is changed relative to current pos,
    //! otherwise from begin of file
    bool seek(s64 finalPos, bool relativeMovement);


    s64 getPos() const;


    bool openFile(const core::CPath& fileName, bool append);


    const core::CPath& getFileName() const {
        return mFilename;
    }

    s64 getFileSize()const {
        return mFileSize;
    }

protected:
    core::CPath mFilename;
    FILE* mFile;
    s64 mFileSize;

private:
    CFileWriter(const CFileWriter&) = delete;
    CFileWriter& operator=(const CFileWriter&) = delete;
};


} // end namespace io
} // end namespace app

#endif //APP_CFILEWRITER_H

