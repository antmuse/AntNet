#include "CPipe.h"
#if defined(APP_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#endif //APP_PLATFORM_WINDOWS


#if defined(APP_PLATFORM_WINDOWS)

namespace irr {

    CPipe::CPipe() {
        SECURITY_ATTRIBUTES attr;
        attr.nLength              = sizeof(attr);
        attr.lpSecurityDescriptor = NULL;
        attr.bInheritHandle       = FALSE;

        if (!CreatePipe(&mReadHandle, &mWriteHandle, &attr, 0)){
            //("anonymous pipe");
        }
    }


    CPipe::~CPipe() {
        closeAll();
    }


    s32 CPipe::writeBytes(const void* buffer, s32 length){
        APP_ASSERT (mWriteHandle != INVALID_HANDLE_VALUE);

        DWORD bytesWritten = 0;
        if (!WriteFile(mWriteHandle, buffer, length, &bytesWritten, NULL)){
            //("anonymous pipe");
        }
        return bytesWritten;
    }


    s32 CPipe::readBytes(void* buffer, s32 length) {
        APP_ASSERT (mReadHandle != INVALID_HANDLE_VALUE);

        DWORD bytesRead = 0;
        BOOL ok = ReadFile(mReadHandle, buffer, length, &bytesRead, NULL);
        if (ok || GetLastError() == ERROR_BROKEN_PIPE)
            return bytesRead;
        else
            return -1; //("anonymous pipe");
    }


    void* CPipe::getReadHandle() const  {
        return mReadHandle;
    }


    void* CPipe::getWriteHandle() const   {
        return mWriteHandle;
    }


    void CPipe::closeRead()    {
        if (mReadHandle != INVALID_HANDLE_VALUE){
            CloseHandle(mReadHandle);
            mReadHandle = INVALID_HANDLE_VALUE;
        }
    }


    void CPipe::closeWrite()    {
        if (mWriteHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(mWriteHandle);
            mWriteHandle = INVALID_HANDLE_VALUE;
        }
    }


} // namespace irr

#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )

namespace irr{

    CPipe::CPipe() {
        s32 fds[2];
        if (0 == pipe(fds)) {
            mReadFD  = fds[0];
            mWriteFD = fds[1];
        } else {
            //("anonymous pipe");
        }
    }


    CPipe::~CPipe() {
        closeAll();
    }


    s32 CPipe::writeBytes(const void* buffer, s32 length){
        APP_ASSERT (mWriteFD != -1);

        s32 n;
        do {
            n = write(mWriteFD, buffer, length);
        }while (n < 0 && errno == EINTR);

        if (n >= 0){
            return n;
        }else{
            return -1; //("anonymous pipe");
        }
    }


    s32 CPipe::readBytes(void* buffer, s32 length) {
        APP_ASSERT (mReadFD != -1);

        s32 n;
        do {
            n = read(mReadFD, buffer, length);
        }  while (n < 0 && errno == EINTR);

        if (n >= 0){
            return n;
        } else{
            return -1; //("anonymous pipe");
        }
    }


    s32 CPipe::getReadHandle() const  {
        return mReadFD;
    }


    s32 CPipe::getWriteHandle() const{
        return mWriteFD;
    }


    void CPipe::closeRead() {
        if (mReadFD != -1)  {
            close(mReadFD);
            mReadFD = -1;
        }
    }


    void CPipe::closeWrite() {
        if (mWriteFD != -1)  {
            close(mWriteFD);
            mWriteFD = -1;
        }
    }


} // namespace irr

#endif //( APP_PLATFORM_ANDROID )  || ( APP_PLATFORM_LINUX )