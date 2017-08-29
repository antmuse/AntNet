#ifndef APP_CNETPACKET_H
#define APP_CNETPACKET_H

#include "HNetConfig.h"
#include "irrArray.h"

namespace irr {
    namespace net {

        enum ENetDataType {
            ENDT_INVALID = 0,
            ENDT_C8,
            ENDT_C16,
            ENDT_S8,
            ENDT_U8,
            ENDT_S16,
            ENDT_U16,
            ENDT_S32,
            ENDT_U32,
            ENDT_F32,
            ENDT_F64,
            ENDT_STRING
        };

        class CNetPacket {
        public:
            CNetPacket();
            CNetPacket(u32 iSize);
            ~CNetPacket();

            CNetPacket(const CNetPacket& other){
                copyOther(other.getConstPointer(), other.getSize());
            }

            CNetPacket& operator = (const CNetPacket& other){
                copyOther(other.getConstPointer(), other.getSize());
                return *this;
            }

            /**
            *@brief Copy from a socket received buffer.
            *@param buffer Socket received buffer.
            *@param iReceived Buffer length of socket received.
            *@return true If init success, else false.
            */
            bool copyFrom(const c8* buffer, u32 iReceived);

            /**
            *@brief Init a net package.
            *@param iReceived Buffer length of socket received.
            *@return true If init success, else false.
            */
            bool init(u32 iReceived);

            ///Direct access operator
            APP_FORCE_INLINE c8& operator [](u32 index) {
                return mData[getHeadSize()+index];
            }

            ///Direct const access operator
            APP_FORCE_INLINE const c8& operator [](u32 index) const {
                return mData[getHeadSize()+index];
            }


            //APP_FORCE_INLINE void setNode(u32 iStart, u32 iEnd) {
            //    mNodeStart = getPointer() + iStart;
            //    mNodeEnd = getPointer() + iEnd;
            //}


            /**
            *@brief slide node window
            *@return true If init next node success, else false.
            */
            bool slideNode();

            /**
            *@brief clear buffer
            *@param iLeftover The leftover size of the buffer after cleared.
            */
            APP_FORCE_INLINE void setUsed(u32 iLeftover) {
                mData.set_used(getHeadSize()+iLeftover);
            }


            APP_FORCE_INLINE void reallocate(u32 iLeftover) {
                mData.reallocate(getHeadSize()+iLeftover, false);
            }


            APP_FORCE_INLINE u32 getAllocatedSize() const {
                return mData.allocated_size();
            }


            APP_FORCE_INLINE c8* getWritePointer() {
                return mRequestSize ? getPointer() + getSize() : getPointer();
            }


            APP_FORCE_INLINE u32 getWriteSize() const {
                return mRequestSize ? mRequestSize : getAllocatedSize();
            }


            APP_FORCE_INLINE bool isEmpty() const {
                return getSize()<=getHeadSize();
            }


            APP_FORCE_INLINE bool isEnd() {
                return mCurrent>=mNodeEnd;
            }

            /**
            *@brief Close package.
            *A package must be closed/reclosed when new data added.
            */
            void finish();


            void clear();


            APP_FORCE_INLINE c8* getCurrentPointer() {
                return mCurrent;
            }


            APP_FORCE_INLINE c8* getPointer() {
                return mData.pointer();
            }


            APP_FORCE_INLINE const c8* getConstPointer() const {
                return mData.const_pointer();
            }


            APP_FORCE_INLINE u32 getSize() const {
                return mData.size();
            }


            APP_FORCE_INLINE u32 getHeadSize() const {
                return sizeof(u32);
            }


            void seek(u32 iPos){
                iPos += getHeadSize();
                iPos = core::min_<u32>(iPos, u32(mNodeEnd-mNodeStart));
                mCurrent = mNodeStart + iPos;
            }



            ///add data type
            APP_FORCE_INLINE u32 add(ENetDataType it){
                mData.push_back(u8(it));
                return mData.size();
            }

            /**
            *@brief add an other CNetPacket.
            *@note Must call finish() before add other pack, and can't call finish() again after call this function.
            *@ setp flow: CNetPacket pack1, pack2;
            *@ step1: pack1.finish();
            *@ step2: pack2.finish();
            *@ step3: pack1.add(pack2); //don't call pack1.finish(); again.
            *@ step4: send(pack1.getPointer(), pack1.getSize());
            *@return package total size.
            */
            u32 add(const CNetPacket& pack);

            ///add string or memery buffer
            u32 add(const c8* iStart, u32 iLength);

            ///add string
            u32 add(const c8* iStart);

            ///add string
            u32 add(const c8* iStart, c8 iEnd){
                const c8* end = iStart;
                while((*end) != iEnd){
                    ++end;
                }
                return add(iStart, u32(end-iStart));
            }

            ///add 8bit
            u32 add(s8 it);
            ///add 8bit
            u32 add(u8 it);
            ///add 16bit
            u32 add(s16 it);
            ///add 16bit
            u32 add(u16 it);
            ///add 32bit
            u32 add(s32 it);
            ///add 32bit
            u32 add(u32 it);
            ///add 32bit
            u32 add(f32 it);


            APP_FORCE_INLINE ENetDataType readType(){
                return (ENetDataType)(*mCurrent++);
            }

            APP_FORCE_INLINE s8 readS8(){
                return (*mCurrent++);
            }

            APP_FORCE_INLINE u8 readU8(){
                return *((u8*)(mCurrent++));
            }

            s16 readS16();

            u16 readU16();

            s32 readS32();

            u32 readU32();

            f32 readF32();


            u32 readString(core::array<c8>& out);


            APP_FORCE_INLINE u32 readNodeSize() const {
                return readU32(mNodeStart);
            }


        private:

            core::array<c8> mData;
            u32 mRequestSize;
            c8* mCurrent;
            c8* mNodeStart;
            c8* mNodeEnd;

            ///write package total size in head
            APP_FORCE_INLINE void writeSize(u32 iSize);

            u32 readU32(const c8* iStart) const;

            u32 addBuffer(const c8* iData, u32 iLength);

            APP_FORCE_INLINE void copyOther(const c8* buffer, u32 iReceived){
                mData.reallocate(iReceived, false);
                memcpy(getPointer(), buffer, iReceived);
                mData.set_used(iReceived);
                mNodeStart = getPointer();
                mNodeEnd = mNodeStart + getSize();
                mRequestSize = 0;
                seek(0);
            }

        };

    }// end namespace net
}// end namespace irr

#endif //APP_CNETPACKET_H