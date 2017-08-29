/**
*@file CStreamFile.h
*@brief Memory file for streams.
*/

#ifndef APP_CSTREAMFILE_H
#define APP_CSTREAMFILE_H

#include "IReferenceCounted.h"
#include "CQueueRing.h"

namespace irr{
    namespace io{

        /**
        *@brief Class for reading memory streams.
        */
        class CStreamFile : public virtual IReferenceCounted{
        public:
            /**
            *@brief Default constructor of memory buffer stream file.
            *default block count = 8, default block size = 4096 - sizeof(SQueueRingFreelock)
            */
            CStreamFile();

            /**
            *@brief Constructor of memory buffer stream file.
            *@param iBlockCount Number of blocks to create.
            *@param iBlockSize Size of each block.
            */
            CStreamFile(u32 iBlockCount, u32 iBlockSize);

            ///Destructor
            virtual ~CStreamFile();

            /**
            *@return true if not empty now, else false.
            */
            APP_INLINE bool isReadable() const;

            /**
            *@return true if not full now, else false.
            */
            APP_INLINE bool isWritable() const;

            /**
            *@brief Read buffer
            *@param buffer User's cache.
            *@param sizeToRead User's cache size.
            *@return The size was readed.
            */
            u32 read(s8* buffer, u32 sizeToRead);

            /**
            *@brief Write buffer
            *@param buffer User's data.
            *@param sizeToWrite User's data size.
            *@param force Force to write in.
            *@return The size was writed.
            */
            u32 write(const s8* buffer, u32 sizeToWrite, bool force = false);

            /**
            *@brief Append cache block with flag ECF_ORIGINAL
            *@param iBlockCount Number of blocks to create.
            *@param iBlockSize Size of block, 0: will use current block size.
            *@return true if success, else false.
            */
            bool append(u32 iBlockCount, u32 iBlockSize = 0);

            /**
            *@brief Get a writable cache block.
            *@param cache Pointer of the writable block pointer.
            *@param size Pointer of writable size.
            *@param need Cache size user needed, will create new cache block if need>0 and CStreamFile is full now.
            *@return true if success to got a writable block, else false.
            *@note User must call closeWrite() to return block to it's original holder if success to got a writable block.
            *eg:
            *@code
            s8* block;
            u32 size;
            if(CStreamFile->openWrite(&block, &size)){
                u32 realsize = user_size >= size ? size : user_size;
                memcpy(block, user_cache, realsize);
                CStreamFile->closeWrite(realsize);
            }
            *@endcode
            */
            bool openWrite(s8** cache, u32* size, u32 need = 0);

            /**
            *@brief Return a cache block to it's original holder after writed.
            *@param size Size of bytes that had been writed by user.
            */
            void closeWrite(u32 size);

            /**
            *@brief Get a readable cache block.
            *@param cache Pointer of the readable block pointer.
            *@param size Pointer of readable size.
            *@return true if success to got a readable block, else false.
            *@note User must call closeRead() to return block to it's original holder if success to got a readable block.
            *eg: 
            *@code
            s8* block;
            u32 size;
            if(CStreamFile->openRead(&block, &size)){
                u32 realsize = user_size >= size ? size : user_size;
                memcpy(user_cache, block, realsize);
                CStreamFile->closeRead(realsize);
            }
            *@endcode
            */
            bool openRead(s8** cache, u32* size);

            /**
            *@brief Return a cache block to it's original holder after readed.
            *@param size Size of bytes that had been consumed by user.
            */
            void closeRead(u32 size);

            /**
            *@brief Change cache hold type.
            *Cache block with flag ECF_APPENDED will hold always if hold type=false, else will be deleted after used.
            *Cache block with flag ECF_ORIGINAL will hold always whether hold type is true or false. 
            *@param type hold type.
            */
            void setHoldType(bool type){
                mOnlyHoldOriginal = type;
            }

            /**
            *@brief Change cache block size.
            *@param iSize block size.
            */
            void setBlockSize(u32 iSize){
                mBlockSize = iSize;
            }

            ///Get current block size.
            APP_INLINE u32 getBlockSize() const{
                return mBlockSize;
            }

            ///Get current block count.
            APP_INLINE s32 getBlockCount() const{
                return mBlockCreated-mBlockDeleted;
            }

            /**
            *@brief Clear all data and delete cache block with flag ECF_APPENDED if hold type is true.
            *@see setHoldType()
            */
            void clear();

        private:
            ///flag of cache block
            enum ECacheFlag{
                ///cache block with flag ECF_APPENDED will hold always if block hold type=false, else will be deleted after used.
                ///User can change the hold type by call setHoldType().
                ECF_APPENDED = 0,

                ///cache block with flag ECF_ORIGINAL will hold always
                ECF_ORIGINAL = 1
            };

            SQueueRingFreelock* goNextRead(SQueueRingFreelock* current);

            SQueueRingFreelock* createBlock(u32 iBlockSize);

            /**
            *@brief Create cache block with flag ECF_ORIGINAL
            *@param iBlockCount Number of blocks to create.
            *@param iBlockSize Size of each block.
            *@return true if success, else false.
            */
            bool init(u32 iBlockCount, u32 iBlockSize);

            ///delete all cache blocks
            void releaseAll();

            ///cache block hold type
            ///true: only hold original blocks witch has flag ECF_ORIGINAL,  
            ///blocks with other flag(eg:ECF_APPENDED) will be deleted after used.
            ///false: hold all blocks, include ECF_APPENDED and ECF_ORIGINAL
            bool mOnlyHoldOriginal;

            s32 mBlockDeleted;                                          ///<deleted cache block count
            s32 mBlockCreated;                                          ///<created cache block count
            u32 mBlockSize;                                                 ///<each block size, default is 1024 bytes.
            SQueueRingFreelock* mReadHandle;        ///<current read position
            SQueueRingFreelock* mWriteHandle;       ///<current write position
        };

    } // end namespace io
} // end namespace irr

#endif //APP_CSTREAMFILE_H

