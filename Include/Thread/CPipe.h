/**
*@file CPipe.h
*@brief This file defined a pipe for processes.
*@author antmuse@live.cn
*@date 2014-11-12
*/

#ifndef APP_CPIPE_H
#define APP_CPIPE_H

#include "HConfig.h"
#include "IReferenceCounted.h"


namespace irr {

/**
*@class CPipe
*@brief A pipe for processes.
*/
class CPipe : public IReferenceCounted {
public:
    CPipe();

    ~CPipe();

    s32 writeBytes(const void* buffer, s32 length);

    s32 readBytes(void* buffer, s32 length);

    void closeRead();

    void closeWrite();

    /**
    *@brief Close both read and write.
    */
    void closeAll() {
        closeRead();
        closeWrite();
    }

#if defined(APP_PLATFORM_WINDOWS)
    void* getReadHandle() const;
    void* getWriteHandle() const;
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    s32 getReadHandle() const;
    s32 getWriteHandle() const;
#endif //APP_PLATFORM_WINDOWS



private:

#if defined(APP_PLATFORM_WINDOWS)
    void* mReadHandle;
    void* mWriteHandle;
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
    s32 mReadFD;
    s32 mWriteFD;
#endif //APP_PLATFORM_WINDOWS
};


} // namespace irr


#endif // APP_CPIPE_H
