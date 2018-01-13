#if defined(_DEBUG) || defined(DEBUG)
#if defined(APP_CHECK_MEM_LEAK)

#include "AppMemoryLeakCheck.h"
#include "CMutex.h"
#include <stdio.h>
#include <stdlib.h>

/**********************************************************************************
#include "HMemoryLeakCheck.h"
#ifdef APP_CHECK_MEM_LEAK
#define new new(__FILE__, __LINE__)
#define malloc(_SIZE_) AppMalloc(_SIZE_, __FILE__, __LINE__)
#define free(_POS_)    AppFree(_POS_)
#define realloc(_POS_, _SIZE_) AppRealloc(_POS_, _SIZE_, __FILE__, __LINE__)
#endif  //APP_CHECK_MEM_LEAK
**********************************************************************************/

struct SMemoryNode {
    size_t mSize;
    unsigned int mLine;
    const char* mFile;
    SMemoryNode* mPrevious;
    SMemoryNode* mNext;
};

unsigned int mTotalCreated = 0;
unsigned int mTotalDeleted = 0;
irr::CMutex mMutex;
SMemoryNode* mHead = 0;

//Inner functions
void* AppCreate(size_t size, const char* file, unsigned int line) {
    irr::CAutoLock au(mMutex);

    SMemoryNode* node = (SMemoryNode*) ::malloc(sizeof(SMemoryNode) + size);
    assert(node);

    if(!node) {
        return 0;
    }

    node->mFile = file;
    node->mLine = line;
    node->mSize = size;

    if(mHead) {
        node->mNext = mHead;
        node->mPrevious = 0;
        mHead->mPrevious = node;
    } else {
        node->mNext = 0;
        node->mPrevious = 0;
    }

    mHead = node;
    ++mTotalCreated;
    return ((char*) node) + sizeof(SMemoryNode);
}


//Inner functions
void AppDelete(void* position) {
    if(0 == position) {
        return;
    }
    irr::CAutoLock au(mMutex);
    if(!mHead) {
        printf("@AppDelete: head is invalid.\n");
        return;
    }

    SMemoryNode* node = (SMemoryNode*) (((char*) position) - sizeof(SMemoryNode));

    if(node == mHead) {
        mHead = mHead->mNext;
        if(mHead) {
            mHead->mPrevious = 0;
        }
    } else if(0 == node->mNext) {
        node->mPrevious->mNext = 0;
    } else {
        node->mNext->mPrevious = node->mPrevious;
        node->mPrevious->mNext = node->mNext;
    }

    ::free(node);
    ++mTotalDeleted;
}


//Inner functions
void* AppRecreate(void* user, size_t size, const char* file, unsigned int line) {
    if(!user) {
        return AppCreate(size, file, line);
    }
    SMemoryNode* old = (SMemoryNode*) (((char*) user) - sizeof(SMemoryNode));
    if(old->mSize >= size) {
        return user;
    }
    char* newmem = (char*) AppCreate(size, file, line);
    ::memcpy(newmem, user, old->mSize);
    AppDelete(user);
    return newmem;
}


void AppPrintMemoryCheckResult() {
    irr::CAutoLock au(mMutex);
    unsigned int count = 0;
    size_t leakedsz = 0;
    SMemoryNode* node = mHead;
    while(node) {
        printf("@AppPrintMemoryCheckResult: [%s] [line=%u] [size=%zu]\n",
            node->mFile,
            node->mLine,
            node->mSize);
        ++count;
        leakedsz += node->mSize;
        node = node->mNext;
    }

    printf("@AppPrintMemoryCheckResult: [leftover=%u,deleted=%u,created=%u][leaked=%zu]\n",
        count,
        mTotalDeleted,
        mTotalCreated,
        leakedsz);
}


void* AppRealloc(void* pos, size_t size, const char* file, unsigned int line) {
    return AppRecreate(pos, size, file, line);
}


void* AppMalloc(size_t size, const char* file, unsigned int line) {
    return AppCreate(size, file, line);
}


void AppFree(void* user) {
    AppDelete(user);
}


void* operator new(size_t size) {
    return AppCreate(size, __FILE__, __LINE__);
}


void* operator new(size_t size, const char* file, unsigned int line) {
    return AppCreate(size, file, line);
}


void* operator new[](size_t size, const char* file, unsigned int line) {
    return AppCreate(size, file, line);
}


void operator delete(void* ptr) {
    AppDelete(ptr);
}


void operator delete[](void* ptr) {
    AppDelete(ptr);
}


#endif //APP_CHECK_MEM_LEAK
#endif //_DEBUG