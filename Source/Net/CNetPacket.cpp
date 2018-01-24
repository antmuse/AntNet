#include "CNetPacket.h"
#include "IUtility.h"
#include <string>

namespace irr {
namespace net {


void CNetPacket::SHead::encode(u8 id, u8 type, u16 size) {
#if defined(APP_ENDIAN_BIG)
    mSize = IUtility::swapByte(size);
#else
    mSize = size;
#endif
    mID = id;
    mType = type;
}

void CNetPacket::SHead::decode() {
#if defined(APP_ENDIAN_BIG)
    u16* ret = ((u16*) (this));
    *ret = IUtility::swapByte(*ret);
#endif
}


CNetPacket::CNetPacket() :
    mDataSize(0),
    mAllocatedSize(APP_NET_MAX_BUFFER_LEN) {
    mData = (c8*) ::malloc(mAllocatedSize);
    APP_ASSERT(mData);
    seek(0);
}


CNetPacket::CNetPacket(u32 iSize) :
    mDataSize(0),
    mAllocatedSize(iSize) {
    if(mAllocatedSize < 8) {
        mAllocatedSize = 8;
    }
    mData = (c8*) ::malloc(mAllocatedSize);
    APP_ASSERT(mData);
    seek(0);
}


CNetPacket::~CNetPacket() {
    ::free(mData);
}


void CNetPacket::shrink(u32 maximum) {
    if(mAllocatedSize > maximum) {
        ::free(mData);
        mData = (c8*) ::malloc(maximum);
        APP_ASSERT(mData);
        mAllocatedSize = maximum;
    }
    mDataSize = 0;
    mCurrent = mData;
}


void CNetPacket::reallocate(u32 size) {
    APP_ASSERT(size <= 1024 * 1024 * 1024);//assume <= 1G
    if(mAllocatedSize < size) {
        size += (mAllocatedSize < 512 ?
            (mAllocatedSize < 8 ? 8 : mAllocatedSize)
            : mAllocatedSize >> 2);

        c8* buffer = (c8*) ::malloc(size);
        APP_ASSERT(buffer);

        if(mDataSize > 0) {
            ::memcpy(buffer, mData, mDataSize);
        }
        ::free(mData);
        mData = buffer;
        mAllocatedSize = size;
    }
}


void CNetPacket::setUsed(u32 size) {
    if(mAllocatedSize < size) {
        reallocate(size);
    }
    mDataSize = size;
    mCurrent = mData;
}


CNetPacket& CNetPacket::operator= (const CNetPacket& other) {
    if(this == &other) {
        return *this;
    }
    mDataSize = 0;
    reallocate(other.getSize());
    ::memcpy(mData, other.getConstPointer(), other.getSize());
    setUsed(other.getSize());
    seek(0);
    return *this;
}


void CNetPacket::setU32(u32 pos, u32 value) {
    IUtility::encodeU32(value, mData + pos);
}


u32 CNetPacket::add(const c8* iStart) {
    return add(iStart, (u32) ::strlen(iStart));
}


u32 CNetPacket::add(const c8* iStart, c8 iEnd) {
    const c8* end = iStart;
    while((*end) != iEnd) {
        ++end;
    }
    return add(iStart, u32(end - iStart));
}


u32 CNetPacket::add(c8 iData) {
    //add(ENDT_S8);
    if(mDataSize + 1 > mAllocatedSize) {
        reallocate(mAllocatedSize + 1);
    }
    *(mData + mDataSize++) = iData;
    return mDataSize;
}


u32 CNetPacket::add(u8 iData) {
    //add(ENDT_U8);
    if(mDataSize + 1 > mAllocatedSize) {
        reallocate(mAllocatedSize + 1);
    }
    *(mData + mDataSize++) = iData;
    return mDataSize;
}


u32 CNetPacket::clear(u32 position) {
    u32 size = mDataSize;
    if(position > 0 && position < size) {
        size = size - position;
        ::memmove(mData, mData + position, size);
        setUsed(size);
        return size;
    } else {
        setUsed(0);
        return 0;
    }
}


u32 CNetPacket::addBuffer(const void* iData, u32 iLength) {
    APP_ASSERT(iData && iLength >= 0);

    u32 osize = mDataSize;
    reallocate(osize + iLength);
    ::memcpy(mData + osize, iData, iLength);
    setUsed(osize + iLength);
    return mDataSize;
}


u32 CNetPacket::add(const c8* iData, u32 iLength) {
    //add(ENDT_STRING);
    add(iLength);
    return addBuffer(iData, iLength);
}


u32 CNetPacket::add(s16 iData) {
    //add(ENDT_S16);
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


u32 CNetPacket::add(u16 iData) {
    //add(ENDT_U16);
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


u32 CNetPacket::add(s32 iData) {
    //add(ENDT_S32);
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


u32 CNetPacket::add(u32 iData) {
    //add(ENDT_U32);
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


u32 CNetPacket::add(f32 iData) {
    //add(ENDT_F32);
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


u32 CNetPacket::add(const CNetPacket& pack) {
    if(pack.getSize() > getWriteSize()) {
        return 0;
    }
    ::memcpy(mData + mDataSize, pack.getConstPointer(), pack.getSize());
    setUsed(mDataSize + pack.getSize());
    return mDataSize;
}


u32 CNetPacket::readString(core::array<c8>& out) const {
    u32 readed = u32(mCurrent - mData) + sizeof(u32);
    if(readed >= mDataSize) {
        //out.setUsed(1);
        //out[0] = '\0';
        return 0;
    }
    u32 nlength = readU32();
    if(nlength > (mDataSize - readed)) {
        //out.setUsed(1);
        //out[0] = '\0';
        return 0;
    }
    out.reallocate(nlength + sizeof(c8));
    ::memcpy(out.pointer(), mCurrent, nlength);
    out.set_used(nlength);
    mCurrent += nlength;
    out.push_back('\0');
    return out.size();
}


s16 CNetPacket::readS16()const {
    s16 ret = *((s16*) (mCurrent));
    mCurrent += sizeof(s16);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return (ret);
}


u16 CNetPacket::readU16()const {
    u16 ret = *((u16*) (mCurrent));
    mCurrent += sizeof(u16);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return (ret);
}


s32 CNetPacket::readS32()const {
    s32 ret = *((s32*) (mCurrent));
    mCurrent += sizeof(s32);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return (ret);
}


u32 CNetPacket::readU32()const {
    u32 ret = *((u32*) (mCurrent));
    mCurrent += sizeof(u32);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return (ret);
}


f32 CNetPacket::readF32()const {
    f32 ret = *((f32*) (mCurrent));
    mCurrent += sizeof(f32);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return (ret);
}


u32 CNetPacket::add(s64 iData) {
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


u32 CNetPacket::add(u64 iData) {
#if defined(APP_ENDIAN_BIG)
    iData = IUtility::swapByte(iData);
#endif
    return addBuffer((c8*) &iData, sizeof(iData));
}


s64 CNetPacket::readS64()const {
    s64 ret = *((s64*) (mCurrent));
    mCurrent += sizeof(s64);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return ret;
}


u64 CNetPacket::readU64()const {
    u64 ret = *((u64*) (mCurrent));
    mCurrent += sizeof(u64);
#if defined(APP_ENDIAN_BIG)
    ret = IUtility::swapByte(ret);
#endif
    return ret;
}


}// end namespace net
}// end namespace irr
