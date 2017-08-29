#include "CNetPacket.h"
#include "IAppLogger.h"
#include "IUtility.h"

namespace irr {
    namespace net {

        CNetPacket::CNetPacket() : mRequestSize(0),
            mNodeStart(0), 
            mNodeEnd(0){
                mData.reallocate(getHeadSize() + APP_NET_MAX_BUFFER_LEN, false);
                clear();
        }

        CNetPacket::CNetPacket(u32 iSize) : mRequestSize(0),
            mNodeStart(0), 
            mNodeEnd(0){
                mData.reallocate(getHeadSize() + iSize, false);
                clear();
        }


        CNetPacket::~CNetPacket(){
        }


        bool CNetPacket::init(u32 iReceived){
            const u32 all = getAllocatedSize();
            u32 packsize =  iReceived + (mRequestSize ? getSize() : 0);
            if(packsize > all){
                clear();
                return false;
            }
            u32 ret = readU32(getPointer());
            if(ret>all){
                clear();
                return false;
            }
            mData.set_used(packsize);
            if(packsize < ret){
                mRequestSize = ret - packsize;
                return false;
            }
            mNodeStart = getPointer();
            mNodeEnd = mNodeStart + ret;
            mRequestSize = 0;
            seek(0);
            return true;
        }


        bool CNetPacket::slideNode(){
            u32 iSize = u32(getPointer()+getSize() - mNodeEnd);
            if(0==iSize){
                return false;
            }
            if(iSize<sizeof(u32)){
                mRequestSize = 1 + getHeadSize() - iSize; ///net package's minimum size = (1 + getHeadSize());
                memmove(getPointer(), mNodeEnd, iSize);
                mData.set_used(iSize);
                mNodeStart = getPointer();
                mNodeEnd = mNodeStart+iSize;
                seek(0);
                return false;
            }
            c8* iStart = mNodeEnd;
            u32 ret = readU32(iStart);
            if(ret <= iSize){
                mNodeStart = iStart;
                mNodeEnd = iStart + ret;
                seek(0);
                return true;
            }else if(ret > getAllocatedSize()){
                clear();
            }else{
                mRequestSize = ret - iSize;
                memmove(getPointer(), iStart, iSize);
                mData.set_used(iSize);
                mNodeStart = getPointer();
                mNodeEnd = mNodeStart+iSize;
                seek(0);
            }
            return false;
        }


        bool CNetPacket::copyFrom(const c8* buffer, u32 iReceived){
            mData.reallocate(iReceived, false);
            memcpy(getPointer(), buffer, iReceived);
            mRequestSize = 0;
            return init(iReceived);
        }


        void CNetPacket::clear() {
            setUsed(0);
            writeSize(0);
            mRequestSize = 0;
            mNodeStart = getPointer();
            mNodeEnd = mNodeStart + getHeadSize();
            mCurrent = mNodeStart;
        }


        APP_FORCE_INLINE void CNetPacket::writeSize(u32 ret){
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            *((u32*)getPointer()) = ret;
        }


        void CNetPacket::finish() {
            writeSize(mData.size());
        }


        u32 CNetPacket::add(const c8* iStart){
            return add(iStart, (u32)strlen(iStart));
        }


        u32 CNetPacket::add(s8 iData){
            //add(ENDT_S8);
            mData.push_back(iData);
            return mData.size();
        }


        u32 CNetPacket::add(u8 iData){
            //add(ENDT_U8);
            mData.push_back(iData);
            return mData.size();
        }

        //inner tool
        u32 CNetPacket::addBuffer(const c8* iData, u32 iLength){
            APP_ASSERT(iData && iLength);

            u32 osize = mData.size();
            reallocate(osize + iLength);
            memcpy(getPointer()+osize, iData, iLength);
            mData.set_used(osize+iLength);
            return mData.size();
        }


        u32 CNetPacket::add(const c8* iData, u32 iLength){
            APP_ASSERT(iData && iLength);
            //add(ENDT_STRING);
            add(iLength);
            return addBuffer(iData, iLength);
        }


        u32 CNetPacket::add(s16 iData){
            //add(ENDT_S16);
#ifndef APP_ENDIAN_BIG
            iData = IUtility::swapByte(iData);
#endif
            return addBuffer((c8*)&iData, sizeof(iData));
        }


        u32 CNetPacket::add(u16 iData){
            //add(ENDT_U16);
#ifndef APP_ENDIAN_BIG
            iData = IUtility::swapByte(iData);
#endif
            return addBuffer((c8*)&iData, sizeof(iData));
        }


        u32 CNetPacket::add(s32 iData){
            //add(ENDT_S32);
#ifndef APP_ENDIAN_BIG
            iData = IUtility::swapByte(iData);
#endif
            return addBuffer((c8*)&iData, sizeof(iData));
        }


        u32 CNetPacket::add(u32 iData){
            //add(ENDT_U32);
#ifndef APP_ENDIAN_BIG
            iData = IUtility::swapByte(iData);
#endif
            return addBuffer((c8*)&iData, sizeof(iData));
        }


        u32 CNetPacket::add(f32 iData){
            //add(ENDT_F32);
#ifndef APP_ENDIAN_BIG
            iData = IUtility::swapByte(iData);
#endif
            return addBuffer((c8*)&iData, sizeof(iData));
        }


        u32 CNetPacket::add(const CNetPacket& pack){
            if(pack.getSize() > (getAllocatedSize() - getSize())){
                return 0;
            }
            memcpy(getPointer()+getSize(), pack.getConstPointer(), pack.getSize());
            mData.set_used(getSize()+pack.getSize());
            return mData.size();
        }


        u32 CNetPacket::readString(core::array<c8>& out) {
            u32 readed = u32(mCurrent-getPointer()) + sizeof(u32);
            if(readed >= getSize()){
                //out.set_used(1);
                //out[0] = '\0';
                return 0;
            }
            u32 nlength = readU32();
            if(nlength > (getSize() - readed)){
                //out.set_used(1);
                //out[0] = '\0';
                return 0;
            }
            out.reallocate(nlength+sizeof(c8));
            memcpy(out.pointer(), mCurrent, nlength);
            out.set_used(nlength);
            mCurrent += nlength;
            out.push_back('\0');
            return out.size();
        }


        s16 CNetPacket::readS16(){
            s16 ret = *((s16*)(mCurrent));
            mCurrent += sizeof(s16);
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            return (ret);
        }


        u16 CNetPacket::readU16(){
            u16 ret = *((u16*)(mCurrent));
            mCurrent += sizeof(u16);
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            return (ret);
        }


        s32 CNetPacket::readS32(){
            s32 ret = *((s32*)(mCurrent));
            mCurrent += sizeof(s32);
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            return (ret);
        }


        u32 CNetPacket::readU32(){
            u32 ret = *((u32*)(mCurrent));
            mCurrent += sizeof(u32);
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            return (ret);
        }


        u32 CNetPacket::readU32(const c8* iStart) const {
            u32 ret = *((u32*)iStart);
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            return ret;
        }


        f32 CNetPacket::readF32(){
            f32 ret = *((f32*)(mCurrent));
            mCurrent += sizeof(f32);
#ifndef APP_ENDIAN_BIG
            ret = IUtility::swapByte(ret);
#endif
            return (ret);
        }


    }// end namespace net
}// end namespace irr
