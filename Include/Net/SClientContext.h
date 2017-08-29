#ifndef APP_SCLIENTCONTEXT_H
#define APP_SCLIENTCONTEXT_H

#include "INetServer.h"
#include "irrArray.h"
#include "CNetPacket.h"
#include "CNetProtocal.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <Windows.h>

namespace irr {
    namespace net {

        ///Socket IO contex
        struct SContextIO {
            EOperationType mOperationType;     ///<Current operation type.
            u32 mID;
            DWORD mFlags;                                        ///<send or receive flags
            DWORD mBytes;										///<Transfered buffer bytes
            OVERLAPPED     mOverlapped;             ///<Used for each operation of every socket          
            WSABUF       mBuffer;								///<Pointer & cache size, point to the real cache.

            SContextIO() : mFlags(0),
                mOperationType(EOP_NULL),
                mBytes(0) {
                    memset(&mOverlapped, 0, sizeof(mOverlapped)); 
                    memset(&mBuffer, 0, sizeof(mBuffer)); 
            }

            ~SContextIO();

            void init(){
                mFlags = 0;
                mOperationType = EOP_NULL;
                mBytes = 0;
                memset(&mOverlapped, 0, sizeof(mOverlapped)); 
                memset(&mBuffer, 0, sizeof(mBuffer)); 
            }
        } ;


        ///Socket contex of each client
        struct SClientContext {  
#if defined(APP_DEBUG)
            s32 mCacheCount;
#endif		
            bool mPackComplete;												///<Pack
            bool mSendPosted;
            bool mReceivePosted;
            netsocket      mSocket;												 ///<Socket of client
            sockaddr_in mClientAddress;                               ///<Client address
            SQueueRingFreelock* mReadHandle;				///<Send cache read handle
            SQueueRingFreelock* mWriteHandle;				///<Send cache write handle
            SQueueRingFreelock* mInnerData;					///<Inner data list for send
            CNetPacket  mNetPack;										///<The receive cache.
            SContextIO mSender;												///<IO handles
            SContextIO mReceiver;											///<IO handles
            core::array<SContextIO> mAllContextIO;			///<All IO handles of others, eg: Accpet IO

            SClientContext();

            ~SClientContext();

            void setCache(s32 iCount, s32 perSize = 1024-sizeof(SQueueRingFreelock));
            SQueueRingFreelock* createNetpack(s32 iSize);
            void releaseNetpack(SQueueRingFreelock* pack);
        };

        ///Socket contex of each client
        struct SClientContextUDP{  
            u32 mTime;
            sockaddr_in mClientAddress;                               ///<Client address
            CNetProtocal mProtocal;
            CNetPacket  mNetPack;										///<The receive cache.

            SClientContextUDP();

            ~SClientContextUDP(){
            }
        };

    }// end namespace net
}// end namespace irr


#endif //APP_PLATFORM_WINDOWS


#endif //APP_SCLIENTCONTEXT_H