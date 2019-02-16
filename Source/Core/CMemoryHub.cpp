#include "CMemoryHub.h"
#include "HAtomicOperator.h"

namespace irr {

CMemoryHub::CMemoryHub() :
    mReferenceCount(1) {

    mPool128.setPageSize(1024 * 16);
    mPool512.setPageSize(2048 * 16);
    mPool2048.setPageSize(4096 * 16);
    mPool8192.setPageSize(8192 * 16);
}


CMemoryHub::~CMemoryHub() {
}

void CMemoryHub::grab() {
    APP_ASSERT(mReferenceCount > 0);
    AppAtomicIncrementFetch(&mReferenceCount);
}

void CMemoryHub::drop() {
    s32 ret = AppAtomicDecrementFetch(&mReferenceCount);
    APP_ASSERT(ret >= 0);
    if(0 == ret) {
        delete this;
    }
}


void CMemoryHub::setPageSize(s32 size) {
    mPool128.setPageSize(size);
    mPool512.setPageSize(size);
    mPool2048.setPageSize(size);
    mPool8192.setPageSize(size);
}


c8 *CMemoryHub::allocate(u32 bytesWanted, u32 align/* = sizeof(void*)*/) {
    align = align < (u32)sizeof(SMemHead) ? (u32)sizeof(SMemHead) : align;
    bytesWanted += align;
    bytesWanted = APP_ALIGN_DATA(bytesWanted, align);
#ifdef APP_DISABLE_BYTE_POOL
    return malloc(bytesWanted);
#endif
    c8* out;
    if(bytesWanted <= 128) {
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex128.lock();
#endif
        out = (c8*) mPool128.allocate();
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex128.unlock();
#endif
        return getUserPointer(out, align, EMT_128);
    }
    if(bytesWanted <= 512) {
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex512.lock();
#endif
        out = (c8*) mPool512.allocate();
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex512.unlock();
#endif
        return getUserPointer(out, align, EMT_512);
    }
    if(bytesWanted <= 2048) {
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex2048.lock();
#endif
        out = (c8*) mPool2048.allocate();
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex2048.unlock();
#endif
        return getUserPointer(out, align, EMT_2048);
    }
    if(bytesWanted <= 8192) {
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex8192.lock();
#endif
        out = (c8*) mPool8192.allocate();
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex8192.unlock();
#endif
        return getUserPointer(out, align, EMT_8192);
    }

    out = (c8*) ::malloc(bytesWanted + 1);
    return getUserPointer(out, align, EMT_DEFAULT);
}


void CMemoryHub::release(void* data) {
    APP_ASSERT(data);
    if(!data) {
        return;
    }
#ifdef APP_DISABLE_BYTE_POOL
    ::free(data);
#endif
    c8* realData;
    switch(getRealPointer(data, realData)) {
    case EMT_128:
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex128.lock();
#endif
        mPool128.release((u8(*)[128]) realData);
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex128.unlock();
#endif
        break;
    case EMT_512:
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex512.lock();
#endif
        mPool512.release((u8(*)[512]) realData);
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex512.unlock();
#endif
        break;
    case EMT_2048:
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex2048.lock();
#endif
        mPool2048.release((u8(*)[2048]) realData);
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex2048.unlock();
#endif
        break;
    case EMT_8192:
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex8192.lock();
#endif
        mPool8192.release((u8(*)[8192]) realData);
#ifdef APP_THREADSAFE_MEMORY_POOL
        mMutex8192.unlock();
#endif
        break;
    case EMT_DEFAULT:
        ::free(realData);
        break;
    default:
        APP_ASSERT(0);
        break;
    }
}


void CMemoryHub::clear() {
#ifdef APP_THREADSAFE_MEMORY_POOL
    mPool128.clear();
    mPool512.clear();
    mPool2048.clear();
    mPool8192.clear();
#endif //APP_THREADSAFE_MEMORY_POOL
}

}//namespace irr