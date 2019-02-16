#ifndef ANTMUSE_CMEMORYHUB_H
#define ANTMUSE_CMEMORYHUB_H

#include "CMemoryPool.h"
#include "CMutex.h"

// #define APP_DISABLE_BYTE_POOL
#define APP_THREADSAFE_MEMORY_POOL

namespace irr {

///allocate some number of bytes from pools.  Uses the heap if necessary.
class CMemoryHub {
public:
    CMemoryHub();

    ~CMemoryHub();

    // Should be at least 8 times bigger than 8192
    void setPageSize(s32 size);

    c8* allocate(u32 bytesWanted, u32 align = sizeof(void*));

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
#ifdef APP_THREADSAFE_MEMORY_POOL
    CMutex mMutex128;
    CMutex mMutex512;
    CMutex mMutex2048;
    CMutex mMutex8192;
#endif

private:
    s32 mReferenceCount;

    APP_FORCE_INLINE c8* getUserPointer(const c8* real, const u32 align, const EMemType tp)const {
        c8* user = APP_ALIGN_POINTER(real, align);
        user = user >= real + sizeof(SMemHead) ? user : user + align;
        SMemHead& hd = *reinterpret_cast<SMemHead*>(user - sizeof(SMemHead));
        hd.mHeadSize = (u8) (user - real);
        hd.mMemTypte = tp;
        return user;
    }

    APP_FORCE_INLINE const EMemType getRealPointer(const void* user, c8*& real)const {
        const SMemHead& hd = *(reinterpret_cast<const SMemHead*>(((c8*) user) - sizeof(SMemHead)));
        real = ((c8*) user) - hd.mHeadSize;
        return (EMemType)hd.mMemTypte;
    }
};

}//namespace irr


#endif // ANTMUSE_CMEMORYHUB_H
