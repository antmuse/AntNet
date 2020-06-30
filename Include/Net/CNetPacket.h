#ifndef APP_CNETPACKET_H
#define APP_CNETPACKET_H

#include "HNetConfig.h"
#include "AppArray.h"

namespace app {
namespace net {

enum ENetDataType {
    ENDT_INVALID = 0,
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
    struct SHead {
        u16 mSize;
        u8 mID;
        u8 mType;
        void encode(u8 id, u8 type, u16 size);
        void decode();
    };
    CNetPacket();

    CNetPacket(u32 iSize);

    ~CNetPacket();

    CNetPacket(const CNetPacket& other) {
        *this = other;
    }

    CNetPacket& operator= (const CNetPacket& other);

    void reallocate(u32 size);

    ///Direct access operator
    APP_FORCE_INLINE s8& operator[](u32 index) {
        return mData[index];
    }

    ///Direct const access operator
    APP_FORCE_INLINE const s8& operator[](u32 index) const {
        return mData[index];
    }


    void setU32(u32 pos, u32 value);

    /**
    *@brief Set the used size
    *@param size The used size(exclude head size).
    */
    void setUsed(u32 size);


    /**
    *@brief Clear all data and shrink cache size if current cache is more bigger,
    * or just clear all data if current cache is not bigger than maximum.
    *@param maximum The max cache size.
    */
    void shrink(u32 maximum);


    APP_FORCE_INLINE u32 getAllocatedSize() const {
        return mAllocatedSize;
    }


    APP_FORCE_INLINE s8* getWritePointer()const {
        return mData + mDataSize;
    }


    APP_FORCE_INLINE u32 getWriteSize() const {
        return mAllocatedSize - mDataSize;
    }


    APP_FORCE_INLINE s8* getReadPointer()const {
        return mCurrent;
    }

    APP_FORCE_INLINE u32 getReadSize() const {
        return u32(getEnd() - mCurrent);
    }

    APP_FORCE_INLINE s8* getEnd() const {
        return mData + mDataSize;
    }


    APP_FORCE_INLINE s8* getPointer()const {
        return mData;
    }


    APP_FORCE_INLINE const s8* getConstPointer() const {
        return mData;
    }


    APP_FORCE_INLINE u32 getSize() const {
        return mDataSize;
    }

    /**
    *@brief Move read position.
    *@param iPos Seek position.
    *@param absolute Absolutive position if true, else relative position of current.
    */
    void seek(s32 iPos, bool absolute = true) const {
        if(absolute) {
            mCurrent = mData + core::min_<u32>(iPos, mDataSize);
        } else {
            mCurrent += iPos;
            mCurrent = core::clamp<s8*>(mCurrent, mData, getEnd());
        }
    }


    bool isEnd() const {
        return mCurrent >= getEnd();
    }

    ///add data type
    APP_FORCE_INLINE u32 add(ENetDataType it) {
        return add(u8(it));
    }

    /**
    *@brief add an other CNetPacket.
    *@param pack Other data buffer.
    *@return package total size.
    */
    u32 add(const CNetPacket& pack);

    ///add string or memery buffer
    u32 add(const s8* iStart, u32 iLength);

    ///add string
    u32 add(const s8* iStart);

    ///add string or memery buffer
    u32 add(const s8* iStart, s8 iEnd);

    u32 add(s8 it);
    u32 add(u8 it);
    u32 add(s16 it);
    u32 add(u16 it);
    u32 add(s32 it);
    u32 add(u32 it);
    u32 add(f32 it);
    u32 add(s64 it);
    u32 add(u64 it);


    APP_FORCE_INLINE ENetDataType readType()const {
        return (ENetDataType) (*mCurrent++);
    }

    APP_FORCE_INLINE s8 readS8()const {
        return (*mCurrent++);
    }

    APP_FORCE_INLINE u8 readU8()const {
        return *((u8*) (mCurrent++));
    }

    s16 readS16()const;
    u16 readU16()const;
    s32 readS32()const;
    u32 readU32()const;
    s64 readS64()const;
    u64 readU64()const;
    f32 readF32()const;

    u32 readString(core::TArray<s8>& out)const;

    u32 addBuffer(const void* iData, u32 iLength);

    /**
    *@brief Clear consumed buffer and move leftover.
    *@note This function will move the leftover buffer to front if it's not zero.
    * So, user should consume as much buffer as they can.
    *@param iSize Consumed size.
    *@return The leftover size.
    */
    u32 clear(u32 iSize);

    /**
    *@brief Clear consumed buffer and move leftover.
    *@note This function will move the leftover buffer to front if it's not zero.
    * So, user should consume as much buffer as they can.
    *@param iEnd End pointer of consumed buffer .
    *@return The leftover size.
    */
    APP_FORCE_INLINE u32 clear(const s8* const iEnd) {
        return clear(u32(iEnd - mData));
    }

protected:
    u32 mDataSize;
    u32 mAllocatedSize;
    mutable s8* mCurrent;
    s8* mData;
};

}// end namespace net
}// end namespace app

#endif //APP_CNETPACKET_H