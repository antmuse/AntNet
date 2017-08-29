#include "CNamedMutex.h"

#if defined( APP_PLATFORM_WINDOWS )
#include <Windows.h>
#endif

#if defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
#include <pthread.h>
#endif


#if defined( APP_PLATFORM_WINDOWS )

namespace irr {
    CNamedMutex::CNamedMutex(const io::path& iName) : mMutex(0),
        mName(iName){
            mMutex = CreateMutex(0, FALSE, mName.c_str());
            /*if (!mMutex)  throw ("cannot create named mutex", mName);*/
    }


    CNamedMutex::~CNamedMutex(){
        CloseHandle(mMutex);
    }


    void CNamedMutex::lock(){
        switch (WaitForSingleObject(mMutex, INFINITE)) {
        case WAIT_OBJECT_0:
            return;
        case WAIT_ABANDONED:
            //("cannot lock named mutex (abadoned)", mName);
            break;
        default:
            //("cannot lock named mutex", mName);
            break;
        }
    }


    bool CNamedMutex::tryLock(){
        switch (WaitForSingleObject(mMutex, 0)) {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            break;
        case WAIT_ABANDONED:
            //("cannot lock named mutex (abadoned)", mName);
            break;
        default:
            //("cannot lock named mutex", mName);
            break;
        }
        return false;
    }


    void CNamedMutex::unlock(){
        ReleaseMutex(mMutex);
    }

}//irr
#elif defined( APP_PLATFORM_ANDROID )  || defined( APP_PLATFORM_LINUX )
namespace irr {



}//irr
#endif //APP_PLATFORM_LINUX
