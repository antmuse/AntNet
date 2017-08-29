#include <stdlib.h>
#include <string.h>
#include "CStreamFile.h"

#define APP_DEFAULT_MAX_BLOCKS      (8)
#define APP_DEFAULT_BLOCK_SIZE          (4096 - sizeof(SQueueRingFreelock))

namespace irr{
    namespace io{

        CStreamFile::CStreamFile() : mOnlyHoldOriginal(true),
            //mBlockSize(1024),
            mBlockCreated(0),
            mBlockDeleted(0),
            mReadHandle(0),
            mWriteHandle(0){
                init(APP_DEFAULT_MAX_BLOCKS, APP_DEFAULT_BLOCK_SIZE);
        }


        CStreamFile::CStreamFile(u32 iBlockCount, u32 iBlockSize) : mOnlyHoldOriginal(true),
            //mBlockSize(iBlockSize),
            mBlockCreated(0),
            mBlockDeleted(0),
            mReadHandle(0),
            mWriteHandle(0) {
                init(iBlockCount, iBlockSize);
        }


        CStreamFile::~CStreamFile(){
            releaseAll();
        }


        bool CStreamFile::openWrite(s8** cache, u32* size, u32 append/* = 0*/){
            APP_ASSERT(mWriteHandle);

            if(isWritable()){
                *cache = mWriteHandle->mData;
                *size = mWriteHandle->mSize;
                return true;
            }

            if(append > 0){
                SQueueRingFreelock* node = createBlock(append);
                if(node){
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


        void CStreamFile::closeWrite(u32 size){
            mWriteHandle->mCurrentPosition = 0;
            mWriteHandle->mUserDataSize = size;
            mWriteHandle->setReadable();
            mWriteHandle = mWriteHandle->getNext();
        }


        bool CStreamFile::openRead(s8** cache, u32* size){
            APP_ASSERT(mReadHandle);

            if(isReadable()){
                *cache = mReadHandle->mData + mReadHandle->mCurrentPosition;
                *size = mReadHandle->mUserDataSize - mReadHandle->mCurrentPosition;
                return true;
            }

            *cache = 0;
            *size = 0;
            return false;
        }


        void CStreamFile::closeRead(u32 size){
            mReadHandle->mCurrentPosition += size;
            if(mReadHandle->mCurrentPosition >= mReadHandle->mUserDataSize){
                mReadHandle = goNextRead(mReadHandle);
            }
        }


        APP_INLINE bool CStreamFile::isReadable() const{
            APP_ASSERT(mReadHandle);
            return mReadHandle->isReadable();
        }


        APP_INLINE bool CStreamFile::isWritable() const{
            APP_ASSERT(mWriteHandle);
            return mWriteHandle->isWritable();
        }


        u32 CStreamFile::read(s8* buffer, u32 sizeToRead){
            APP_ASSERT(mReadHandle);

            u32 ret = 0;
            u32 need;
            u32 leftover;
            for(; mReadHandle->isReadable();){
                need = sizeToRead - ret;
                if(0 == need){
                    break;
                }
                leftover = mReadHandle->mUserDataSize - mReadHandle->mCurrentPosition;
                if(leftover > 0){
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


        SQueueRingFreelock* CStreamFile::goNextRead(SQueueRingFreelock* current){
            SQueueRingFreelock* node = current->getNext();
            if(ECF_APPENDED == current->mFlag && mOnlyHoldOriginal){
                current->delink();
                free(current);
                ++mBlockDeleted;
            } else {
                current->setWritable();
            }
            return node;
        }


        u32 CStreamFile::write(const s8* buffer, u32 sizeToWrite, bool force/* = false*/){
            APP_ASSERT(buffer);
            APP_ASSERT(mWriteHandle);

            u32 ret = 0;
            u32 leftover;

            for(; ret < sizeToWrite; ){
                leftover = sizeToWrite - ret;
                if(mWriteHandle->isWritable()){
                    leftover = mWriteHandle->mSize >= leftover ? leftover : mWriteHandle->mSize;
                    memcpy(mWriteHandle->mData, buffer, leftover);
                    mWriteHandle->mCurrentPosition = 0;
                    mWriteHandle->mUserDataSize = leftover;
                    mWriteHandle->setReadable();
                    mWriteHandle = mWriteHandle->getNext();
                } else{
                    if(force){
                        SQueueRingFreelock* node = createBlock(leftover);
                        if(!node){
                            break;
                        }
                        memcpy(node->mData, buffer, leftover);
                        node->mUserDataSize = leftover;
                        node->mCurrentPosition = 0;
                        //node->mFlag = ECF_APPENDED; //ECF_APPENDED is default when node inited.
                        mWriteHandle->pushBack(node);
                        mWriteHandle = node;
                        node->setReadable();
                    } else{
                        break;
                    }
                }

                buffer += leftover;
                ret += leftover;
            }//for
            return ret;
        }


        SQueueRingFreelock* CStreamFile::createBlock(u32 iSize){
            APP_ASSERT(iSize <= (0xFFFFFFFF - sizeof(SQueueRingFreelock)));

            SQueueRingFreelock* ret = (SQueueRingFreelock*)malloc(iSize + sizeof(SQueueRingFreelock));
            if(ret){
                ret->init(iSize);
                ++mBlockCreated;
            }
            return ret;
        }


        bool CStreamFile::append(u32 iCount, u32 iCacheSize){
            iCacheSize = (0 == iCacheSize ? mBlockSize : iCacheSize);

            APP_ASSERT(iCount >= 1);
            APP_ASSERT(iCacheSize >= 128);

            for(u32 i = 0; i < iCount; ++i){
                SQueueRingFreelock* node = createBlock(iCacheSize);
                if(!node){
                    return false;
                }
                node->mFlag = ECF_ORIGINAL;
                mWriteHandle->pushBack(node);
            }
            return true;
        }


        bool CStreamFile::init(u32 iCache, u32 iCacheSize){
            APP_ASSERT(iCache >= 2);
            APP_ASSERT(iCacheSize >= 128);

            mBlockSize = iCacheSize;
            mReadHandle = createBlock(mBlockSize);
            if(!mReadHandle){
                releaseAll();
                return false;
            }
            mWriteHandle = mReadHandle;
            mReadHandle->mFlag = ECF_ORIGINAL;

            for(u32 i = 0; i < iCache; ++i){
                SQueueRingFreelock* node = createBlock(mBlockSize);
                if(!node){
                    releaseAll();
                    return false;
                }
                node->mFlag = ECF_ORIGINAL;
                mReadHandle->pushBack(node);
            }
            return true;
        }


        void CStreamFile::releaseAll(){
            if(!mReadHandle) return;

            SQueueRingFreelock* next;
            for(SQueueRingFreelock* it = mReadHandle->getNext(); it != mReadHandle; it = next){
                next = it->getNext();
                it->delink();
                free((void*)it);
            }
            free((void*)mReadHandle);
            mReadHandle = 0;
            mWriteHandle = 0;
            mBlockCreated = 0;
            mBlockDeleted = 0;
        }


        void CStreamFile::clear(){
            if(!mReadHandle) return;

            for(SQueueRingFreelock* it = mReadHandle->getNext(); it != mReadHandle; it = goNextRead(it)){
            }

            mWriteHandle = mReadHandle;
        }

    } // end namespace io
} // end namespace irr

