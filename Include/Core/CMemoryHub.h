#ifndef ANT_CMEMORYHUB_H
#define ANT_CMEMORYHUB_H

#include "CMemoryPool.h"
#include "CMutex.h"

// #define APP_DISABLE_BYTE_POOL
#define APP_THREADSAFE_MEMORY_POOL

namespace app {

///allocate some number of bytes from pools.  Uses the heap if necessary.
class CMemoryHub {
public:
    CMemoryHub();

    ~CMemoryHub();

    void setPageCount(s32 count);

    s8* allocate(u64 bytesWanted, u64 align = sizeof(void*));

    s8* allocateAndClear(u64 bytesWanted, u64 align = sizeof(void*));

    void release(void* data);

    void clear();

    void grab();

    void drop();

protected:
    enum EMemType {
        EMT_128,
        EMT_256,
        EMT_512,
        EMT_1024,
        EMT_2048,
        EMT_4096,
        EMT_8192,
        EMT_10K,
        EMT_DEFAULT
    };
    struct SMemHead {
        u8 mHeadSize;
        u8 mMemTypte; //@see EMemType
    };
    CMemoryPool128 mPool128;
    CMemoryPool512 mPool512;
    CMemoryPool2048 mPool2048;
    CMemoryPool8192 mPool8192;
    CMemoryPool10K mPool10K;
#ifdef APP_THREADSAFE_MEMORY_POOL
    CMutex mMutex128;
    CMutex mMutex512;
    CMutex mMutex2048;
    CMutex mMutex8192;
    CMutex mMutex10K;
#endif

private:
    s32 mReferenceCount;

    CMemoryHub(const CMemoryHub& it) = delete;
    CMemoryHub& operator=(const CMemoryHub& it) = delete;

    APP_FORCE_INLINE s8* getUserPointer(const s8* real, const u64 align, const EMemType tp)const {
        s8* user = APP_ALIGN_POINTER(real, align);
        user = user >= real + sizeof(SMemHead) ? user : user + align;
        SMemHead& hd = *reinterpret_cast<SMemHead*>(user - sizeof(SMemHead));
        hd.mHeadSize = (u8) (user - real);
        hd.mMemTypte = tp;
        return user;
    }

    APP_FORCE_INLINE const EMemType getRealPointer(const void* user, s8*& real)const {
        const SMemHead& hd = *(reinterpret_cast<const SMemHead*>(((s8*) user) - sizeof(SMemHead)));
        real = ((s8*) user) - hd.mHeadSize;
        return (EMemType) hd.mMemTypte;
    }
};

}//namespace app


#endif // ANTMUSE_CMEMORYHUB_H

