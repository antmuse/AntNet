#ifndef ANTMUSE_CMEMORYPOOL_H
#define ANTMUSE_CMEMORYPOOL_H

#include <stdlib.h>
#include "HConfig.h"
#include "irrTypes.h"

// APP_MEMORY_POOL_MAX_FREE_PAGES must be > 1
#define APP_MEMORY_POOL_MAX_FREE_PAGES 4

//#define APP_DISABLE_MEMORY_POOL

namespace irr {

/// Very fast memory pool for allocating and deallocating structures that don't have constructors or destructors.
/// Contains a list of pages, each of which has an array of the user structures
template <class CMemoryBlockType>
class CMemoryPool {
public:
    struct SMemoryPage;
    struct SMemoryWithPage {
        CMemoryBlockType mUserMemory;
        SMemoryPage* mParentPage;
    };

    struct SMemoryPage {
        SMemoryWithPage** mAvailableStack;
        s32 mAvailableStackSize;
        SMemoryWithPage* mBlock;
        SMemoryPage* mNext;
        SMemoryPage* mPrevious;
    };

    CMemoryPool();

    ~CMemoryPool();

    s32 getBlockSize() const {
        return sizeof(CMemoryBlockType);
    }

    void setPageSize(s32 size); // Defaults to 16*1024 bytes

    CMemoryBlockType* allocate();

    void release(CMemoryBlockType* it);

    void clear();

    s32 getAvailablePagesSize(void) const {
        return mAvailablePagesSize;
    }

    s32 getUnavailablePagesSize(void) const {
        return mUnavailablePagesSize;
    }

    s32 getMemoryPoolPageSize(void) const {
        return mMemoryPoolPageSize;
    }

protected:
    s32 blocksPerPage(void) const;

    //void allocateFirst(void);

    bool initPage(SMemoryPage* page, SMemoryPage* iPrevious);

    // mAvailablePages contains pages which have room to give the user new blocks.  We return these blocks from the head of the list
    // mUnavailablePages are pages which are totally full, and from which we do not return new blocks.
    // Pages move from the head of mUnavailablePages to the tail of mAvailablePages, and from the head of mAvailablePages to the tail of mUnavailablePages
    SMemoryPage* mAvailablePages;
    SMemoryPage* mUnavailablePages;
    s32 mAvailablePagesSize;
    s32 mUnavailablePagesSize;
    s32 mMemoryPoolPageSize;
};


template<class CMemoryBlockType>
CMemoryPool<CMemoryBlockType>::CMemoryPool() {
#ifndef APP_DISABLE_MEMORY_POOL
    //allocateFirst();
    mAvailablePagesSize = 0;
    mUnavailablePagesSize = 0;
    setPageSize(16 * 1024);
#endif
}


template<class CMemoryBlockType>
CMemoryPool<CMemoryBlockType>::~CMemoryPool() {
#ifndef APP_DISABLE_MEMORY_POOL
    clear();
#endif
}


template<class CMemoryBlockType>
void CMemoryPool<CMemoryBlockType>::setPageSize(s32 size) {
    s32 count = (size + sizeof(SMemoryWithPage) - 1) / sizeof(SMemoryWithPage);
    mMemoryPoolPageSize = count * sizeof(SMemoryWithPage);
}


template<class CMemoryBlockType>
CMemoryBlockType* CMemoryPool<CMemoryBlockType>::allocate() {
#ifdef APP_DISABLE_MEMORY_POOL
    return (CMemoryBlockType*) ::malloc(sizeof(CMemoryBlockType));
#else

    if(mAvailablePagesSize > 0) {
        SMemoryPage *curPage = mAvailablePages;
        CMemoryBlockType *retVal = (CMemoryBlockType*) curPage->mAvailableStack[--(curPage->mAvailableStackSize)];
        if(curPage->mAvailableStackSize == 0) {
            --mAvailablePagesSize;
            mAvailablePages = curPage->mNext;
            APP_ASSERT(mAvailablePagesSize == 0 || mAvailablePages->mAvailableStackSize > 0);
            curPage->mNext->mPrevious = curPage->mPrevious;
            curPage->mPrevious->mNext = curPage->mNext;

            if(mUnavailablePagesSize++ == 0) {
                mUnavailablePages = curPage;
                curPage->mNext = curPage;
                curPage->mPrevious = curPage;
            } else {
                curPage->mNext = mUnavailablePages;
                curPage->mPrevious = mUnavailablePages->mPrevious;
                mUnavailablePages->mPrevious->mNext = curPage;
                mUnavailablePages->mPrevious = curPage;
            }
        }

        APP_ASSERT(mAvailablePagesSize == 0 || mAvailablePages->mAvailableStackSize > 0);
        return retVal;
    }//if

    mAvailablePages = (SMemoryPage *) ::malloc(sizeof(SMemoryPage));
    if(mAvailablePages == 0) {
        return 0;
    }
    mAvailablePagesSize = 1;

    if(initPage(mAvailablePages, mAvailablePages) == false) {
        return 0;
    }
    // If this assert hits, we couldn't allocate even 1 mBlock per page. Increase the page size
    APP_ASSERT(mAvailablePages->mAvailableStackSize > 1);

    return (CMemoryBlockType*) mAvailablePages->mAvailableStack[--mAvailablePages->mAvailableStackSize];
#endif
}


template<class CMemoryBlockType>
void CMemoryPool<CMemoryBlockType>::release(CMemoryBlockType *m) {
#ifdef APP_DISABLE_MEMORY_POOL
    ::free(m);
    return;
#else
    // Find the page this mBlock is in and return it.
    SMemoryWithPage *memoryWithPage = (SMemoryWithPage*) m;
    SMemoryPage *curPage = memoryWithPage->mParentPage;

    if(curPage->mAvailableStackSize == 0) {
        // The page is in the unavailable list so move it to the available list
        curPage->mAvailableStack[curPage->mAvailableStackSize++] = memoryWithPage;
        mUnavailablePagesSize--;

        // As this page is no longer totally empty, move it to the end of available pages
        curPage->mNext->mPrevious = curPage->mPrevious;
        curPage->mPrevious->mNext = curPage->mNext;

        if(mUnavailablePagesSize > 0 && curPage == mUnavailablePages)
            mUnavailablePages = mUnavailablePages->mNext;

        if(mAvailablePagesSize++ == 0) {
            mAvailablePages = curPage;
            curPage->mNext = curPage;
            curPage->mPrevious = curPage;
        } else {
            curPage->mNext = mAvailablePages;
            curPage->mPrevious = mAvailablePages->mPrevious;
            mAvailablePages->mPrevious->mNext = curPage;
            mAvailablePages->mPrevious = curPage;
        }
    } else {
        curPage->mAvailableStack[curPage->mAvailableStackSize++] = memoryWithPage;

        if(curPage->mAvailableStackSize == blocksPerPage() &&
            mAvailablePagesSize >= APP_MEMORY_POOL_MAX_FREE_PAGES) {
            // After a certain point, just deallocate empty pages rather than keep them around
            if(curPage == mAvailablePages) {
                mAvailablePages = curPage->mNext;
                APP_ASSERT(mAvailablePages->mAvailableStackSize > 0);
            }
            curPage->mPrevious->mNext = curPage->mNext;
            curPage->mNext->mPrevious = curPage->mPrevious;
            mAvailablePagesSize--;
            ::free(curPage->mAvailableStack);
            ::free(curPage->mBlock);
            ::free(curPage);
        }
    }
#endif
}


template<class CMemoryBlockType>
void CMemoryPool<CMemoryBlockType>::clear() {
#ifdef APP_DISABLE_MEMORY_POOL
    return;
#else
    SMemoryPage *cur, *freed;

    if(mAvailablePagesSize > 0) {
        cur = mAvailablePages;
#ifdef _MSC_VER
#pragma warning(disable:4127)   // conditional expression is constant
#endif
        while(true) {
            free(cur->mAvailableStack);
            free(cur->mBlock);
            freed = cur;
            cur = cur->mNext;
            if(cur == mAvailablePages) {
                free(freed);
                break;
            }
            free(freed);
        }// while
    }//if

    if(mUnavailablePagesSize > 0) {
        cur = mUnavailablePages;
        while(1) {
            free(cur->mAvailableStack);
            free(cur->mBlock);
            freed = cur;
            cur = cur->mNext;
            if(cur == mUnavailablePages) {
                free(freed);
                break;
            }
            free(freed);
        } // while
    }//if

    mAvailablePagesSize = 0;
    mUnavailablePagesSize = 0;
#endif
}


template<class CMemoryBlockType>
s32 CMemoryPool<CMemoryBlockType>::blocksPerPage(void) const {
    return mMemoryPoolPageSize / sizeof(SMemoryWithPage);
}


template<class CMemoryBlockType>
bool CMemoryPool<CMemoryBlockType>::initPage(SMemoryPage* page, SMemoryPage* iPrevious) {
    const s32 bpp = blocksPerPage();
    page->mBlock = (SMemoryWithPage*) ::malloc(mMemoryPoolPageSize);

    if(page->mBlock == 0) {
        return false;
    }
    page->mAvailableStack = (SMemoryWithPage**) ::malloc(sizeof(SMemoryWithPage*)*bpp);
    if(page->mAvailableStack == 0) {
        ::free(page->mBlock);
        return false;
    }
    SMemoryWithPage *curBlock = page->mBlock;
    SMemoryWithPage **curStack = page->mAvailableStack;
    s32 i = 0;
    while(i < bpp) {
        curBlock->mParentPage = page;
        curStack[i] = curBlock++;
        i++;
    }
    page->mAvailableStackSize = bpp;
    page->mNext = mAvailablePages;
    page->mPrevious = iPrevious;
    return true;
}


typedef CMemoryPool<u8[128]> CMemoryPool128;
typedef CMemoryPool<u8[256]> CMemoryPool256;
typedef CMemoryPool<u8[512]> CMemoryPool512;
typedef CMemoryPool<u8[1024]> CMemoryPool1024;
typedef CMemoryPool<u8[2048]> CMemoryPool2048;
typedef CMemoryPool<u8[4096]> CMemoryPool4096;
typedef CMemoryPool<u8[8192]> CMemoryPool8192;
typedef CMemoryPool<u8[10240]> CMemoryPool10K;
}//namespace irr


#endif // ANTMUSE_CMEMORYPOOL_H
