#include <stdlib.h>
#include <string.h>
#include "CStreamFile.h"

#include "HMemoryLeakCheck.h"

#define APP_DEFAULT_MAX_BLOCKS      (8)
#define APP_DEFAULT_BLOCK_SIZE      (4096 - sizeof(SQueueRingFreelock))

namespace irr {
namespace io {

CStreamFile::CStreamFile() : mOnlyHoldOriginal(true),
//mBlockSize(1024),
mBlockCreated(0),
mBlockDeleted(0),
mReadHandle(0),
mWriteHandle(0) {
    init(APP_DEFAULT_MAX_BLOCKS, APP_DEFAULT_BLOCK_SIZE);
}


CStreamFile::CStreamFile(s32 iBlockCount, s32 iBlockSize) : mOnlyHoldOriginal(true),
//mBlockSize(iBlockSize),
mBlockCreated(0),
mBlockDeleted(0),
mReadHandle(0),
mWriteHandle(0) {
    init(iBlockCount, iBlockSize);
}


CStreamFile::~CStreamFile() {
    releaseAll();
}


bool CStreamFile::isEnough(u32 size)const {
    SQueueRingFreelock* wpos = mWriteHandle;
    u32 cache = 0;

    while(cache < size && wpos->isWritable()) {
        cache += (wpos->mSize - wpos->mUserDataSize);
        wpos = wpos->getNext();
    }

    return cache >= size;
}


bool CStreamFile::openWrite(c8** cache, s32* size, s32 append/* = 0*/) {
    APP_ASSERT(mWriteHandle);

    if(isWritable()) {
        *cache = mWriteHandle->mData;
        *size = mWriteHandle->mSize;
        return true;
    }

    if(append > 0) {
        SQueueRingFreelock* node = createBlock(append);
        if(node) {
            *cache = node->mData;
            *size = node->mSize;
            //node->mUserDataSize = 0;
            //node->mCurrentPosition = 0;
            //node->mFlag = ECF_APPENDED; //ECF_APPENDED is default when node inited.
            mWriteHandle->pushBack(node);
            mWriteHandle = node;
            node->setReadable();
            return true;
        }
    }

    *cache = 0;
    *size = 0;
    return false;
}


void CStreamFile::closeWrite(s32 size) {
    mWriteHandle->mCurrentPosition = 0;
    mWriteHandle->mUserDataSize = size;
    mWriteHandle->setReadable();
    mWriteHandle = mWriteHandle->getNext();
}


bool CStreamFile::openRead(c8** cache, s32* size) {
    APP_ASSERT(mReadHandle);

    if(isReadable()) {
        *cache = mReadHandle->mData + mReadHandle->mCurrentPosition;
        *size = mReadHandle->mUserDataSize - mReadHandle->mCurrentPosition;
        return true;
    }

    *cache = 0;
    *size = 0;
    return false;
}


void CStreamFile::closeRead(s32 size) {
    mReadHandle->mCurrentPosition += size;
    if(mReadHandle->mCurrentPosition >= mReadHandle->mUserDataSize) {
        mReadHandle = goNextRead(mReadHandle);
    }
}


APP_INLINE bool CStreamFile::isReadable() const {
    APP_ASSERT(mReadHandle);
    return mReadHandle->isReadable();
}


APP_INLINE bool CStreamFile::isWritable() const {
    APP_ASSERT(mWriteHandle);
    return mWriteHandle->isWritable();
}


s32 CStreamFile::read(c8* buffer, s32 sizeToRead) {
    APP_ASSERT(mReadHandle);

    s32 ret = 0;
    s32 need;
    s32 leftover;
    for(; mReadHandle->isReadable();) {
        need = sizeToRead - ret;
        if(0 == need) {
            break;
        }
        leftover = mReadHandle->mUserDataSize - mReadHandle->mCurrentPosition;
        if(leftover > 0) {
            leftover = leftover >= need ? need : leftover;
            memcpy(buffer, mReadHandle->mData + mReadHandle->mCurrentPosition, leftover);
            mReadHandle->mCurrentPosition += leftover;
            buffer += leftover;
            ret += leftover;
        } else {
            mReadHandle = goNextRead(mReadHandle);
        }
    }
    return ret;
}


s32 CStreamFile::readLine(c8* buffer, s32 maxSize) {
    APP_ASSERT(mReadHandle);

    //empty file
    if(!mReadHandle->isReadable()) {
        return 0;
    }

    SQueueRingFreelock* node = mReadHandle;
    bool find = false;
    const c8* pos;
    s32 linesize = 0;
    s32 leftover;
    s32 got = 0;

    for(; node->isReadable(); node = node->getNext()) {
        leftover = node->mUserDataSize - node->mCurrentPosition;
        pos = node->mData + node->mCurrentPosition;
        for(got = 0; got < leftover && (*pos++ != '\n'); ++got) {
        }
        if(got < leftover) {//find it
            find = true;
            linesize += ++got;
            break;
        } else {
            linesize += got;
            if(linesize > maxSize) {//not enough cache
                return -1;  //return as early as can
            }
        }
    }

    //not enough cache
    if(linesize > maxSize) {
        return -1;
    }

    //not found endcode '\n';
    if(!find) {
        return 0;
    }

    //found it, read the line
    for(; mReadHandle != node; mReadHandle = goNextRead(mReadHandle)) {
        leftover = mReadHandle->mUserDataSize - mReadHandle->mCurrentPosition;
        memcpy(buffer, mReadHandle->mData + mReadHandle->mCurrentPosition, leftover);
        buffer += leftover;
    }
    leftover = mReadHandle->mUserDataSize - mReadHandle->mCurrentPosition;
    memcpy(buffer, mReadHandle->mData + mReadHandle->mCurrentPosition, got);
    mReadHandle->mCurrentPosition += got;
    if(got == leftover) {
        mReadHandle = goNextRead(mReadHandle);
    }

    return linesize;
}


SQueueRingFreelock* CStreamFile::goNextRead(SQueueRingFreelock* current) {
    SQueueRingFreelock* node = current->getNext();
    if(ECF_APPENDED == current->mFlag && mOnlyHoldOriginal) {
        current->delink();
        ::free(current);
        ++mBlockDeleted;
    } else {
        current->setWritable();
    }
    return node;
}


s32 CStreamFile::write(const c8* buffer, s32 sizeToWrite, bool force/* = false*/, bool close/* = true*/) {
    APP_ASSERT(buffer);
    APP_ASSERT(mWriteHandle);

    s32 ret = 0;
    s32 leftover;

    for(; ret < sizeToWrite; ) {
        leftover = sizeToWrite - ret;
        if(mWriteHandle->isWritable()) {
            u32 cachesize = mWriteHandle->mSize - mWriteHandle->mUserDataSize;
            if(0 == cachesize) { //full filled
                mWriteHandle->setReadable();
                mWriteHandle = mWriteHandle->getNext();
                continue;
            }
            leftover = cachesize >= leftover ? leftover : cachesize;
            ::memcpy(mWriteHandle->mData + mWriteHandle->mUserDataSize, buffer, leftover);
            mWriteHandle->mUserDataSize += leftover;
            if(close) {
                mWriteHandle->setReadable();
                mWriteHandle = mWriteHandle->getNext();
            }
        } else {
            if(!force) {
                break;
            }
            SQueueRingFreelock* node = createBlock(leftover);
            if(!node) {
                break;
            }
            ::memcpy(node->mData, buffer, leftover);
            node->mUserDataSize = leftover;
            //node->mFlag = ECF_APPENDED; //ECF_APPENDED is default when node inited.
            mWriteHandle->pushBack(node);
            mWriteHandle = node;
            if(close) {
                node->setReadable();
            }
        }

        buffer += leftover;
        ret += leftover;
    }//for

    return ret;
}


SQueueRingFreelock* CStreamFile::createBlock(s32 iSize) {
    APP_ASSERT(iSize <= (0xFFFFFFFF - sizeof(SQueueRingFreelock)));

    SQueueRingFreelock* ret = (SQueueRingFreelock*) malloc(iSize + sizeof(SQueueRingFreelock));
    if(ret) {
        ret->init(iSize);
        ++mBlockCreated;
    }
    return ret;
}


bool CStreamFile::append(s32 iCount, s32 iCacheSize) {
    iCacheSize = (0 == iCacheSize ? mBlockSize : iCacheSize);

    APP_ASSERT(iCount >= 1);
    APP_ASSERT(iCacheSize >= 128);

    for(s32 i = 0; i < iCount; ++i) {
        SQueueRingFreelock* node = createBlock(iCacheSize);
        if(!node) {
            return false;
        }
        node->mFlag = ECF_ORIGINAL;
        mWriteHandle->pushBack(node);
    }
    return true;
}


bool CStreamFile::init(s32 iCache, s32 iCacheSize) {
    APP_ASSERT(iCache >= 2);
    //APP_ASSERT(iCacheSize >= 128);

    mBlockSize = iCacheSize;
    mReadHandle = createBlock(mBlockSize);
    if(!mReadHandle) {
        releaseAll();
        return false;
    }
    mWriteHandle = mReadHandle;
    mReadHandle->mFlag = ECF_ORIGINAL;

    for(s32 i = 0; i < iCache; ++i) {
        SQueueRingFreelock* node = createBlock(mBlockSize);
        if(!node) {
            releaseAll();
            return false;
        }
        node->mFlag = ECF_ORIGINAL;
        mReadHandle->pushBack(node);
    }
    return true;
}


void CStreamFile::releaseAll() {
    if(!mReadHandle) return;

    SQueueRingFreelock* next;
    for(SQueueRingFreelock* it = mReadHandle->getNext(); it != mReadHandle; it = next) {
        next = it->getNext();
        it->delink();
        free((void*) it);
    }
    free((void*) mReadHandle);
    mReadHandle = 0;
    mWriteHandle = 0;
    mBlockCreated = 0;
    mBlockDeleted = 0;
}


void CStreamFile::clear() {
    if(!mReadHandle) return;

    for(SQueueRingFreelock* it = mReadHandle->getNext();
        it != mReadHandle; it = goNextRead(it)) {
    }

    mReadHandle->setWritable();
    mWriteHandle = mReadHandle;
}


} // end namespace io
} // end namespace irr

