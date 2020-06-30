#ifndef APP_CEVENTPOLLER_H
#define APP_CEVENTPOLLER_H

#include "HConfig.h"

#if defined(APP_PLATFORM_WINDOWS)
namespace app {
namespace net {
class CNetSocket;
}


class CEventPoller {
public:
#if defined(APP_PLATFORM_WINDOWS)
    ///SEvent layout must same as OVERLAPPED_ENTRY's layout
    struct SEvent {
#if defined(APP_OS_64BIT)
        u64 mKey;           ///< PULONG_PTR
        void* mPointer;     ///< LPOVERLAPPED
        u64 mInternal;
#else
        u32 mKey;           ///< PULONG_PTR
        void* mPointer;     ///< LPOVERLAPPED
        u32 mInternal;
#endif
        u32 mBytes;         ///< DWORD, Bytes Transferred
    };
#endif //APP_PLATFORM_WINDOWS

    CEventPoller();

    ~CEventPoller();

    static s32 getError();

    void close();

    /**
    * @return 1 if success or timeout, else -1
    */
    s32 getEvent(SEvent& iEvent, u32 time);

    /**
    * @return count of launched events if success, else -1
    */
    s32 getEvents(SEvent* iEvent, s32 iSize, u32 iTime);

    bool add(void* fd, void* key);

    bool add(const net::CNetSocket& iSock, void* iKey);

    //IOCP does not support remove fd.
    //bool remove(void* fd, void* overlp);

    static bool getResult(void* fd, void* overlp, u32* bytes, u32 wait);

    /**
    *@brief cancel IO of this thread.
    *@param handle The IO handle.
    */
    static bool cancelIO(void* handle);

    static bool cancelIO(const net::CNetSocket& sock);

    /**
    *@brief cancel IO of all thread.
    *@param handle The IO handle.
    *@param overlp 0 to cancel all IO of handle, else just cancel the IO against to \p overlp.
    */
    static bool cancelIO(void* handle, void* overlp);

    static bool cancelIO(const net::CNetSocket& sock, void* overlp);

    static bool hasOverlappedIoCompleted(void* overlp);

    bool postEvent(SEvent& iEvent);

protected:
    void* mHandle;
};


} //namespace app

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include "CNetSocket.h"

namespace app {
//namespace net {
//class CNetSocket;
//}

enum EPollerEvent {
    EPOLLIN = 0x00000001,
    EPOLLPRI = 0x00000002,
    EPOLLOUT = 0x00000004,
    EPOLLERR = 0x00000008,
    EPOLLHUP = 0x00000010,
    EPOLLRDNORM = 0x00000040,
    EPOLLRDBAND = 0x00000080,
    EPOLLWRNORM = 0x00000100,
    EPOLLWRBAND = 0x00000200,
    EPOLLMSG = 0x00000400,
    EPOLLRDHUP = 0x00002000,
    EPOLLWAKEUP = 0x20000000,
    EPOLLONESHOT = 0x40000000,
    EPOLLET = 0x80000000
};

class CEventPoller {
public:
    ///SEvent layout must same as epoll_event's layout
#if defined(APP_PLATFORM_LINUX)
#pragma pack(4)
#endif
    struct SEvent {
        union UData {
            u32 mData32;
            u64 mData64;
            void* mPointer;
        };
        u32 mEvent;
        UData mData;
        void setMessage(u32 msg) {
            mEvent = msg;
        }
    };
#if defined(APP_PLATFORM_LINUX)
#pragma pack()
#endif

    CEventPoller();

    ~CEventPoller();

    static s32 getError();

    /**
    * @return count of launched events if success, or 0 if timeout, else -1
    */
    s32 getEvent(SEvent& iEvent, u32 iTime);

    /**
    * @return count of launched events if success, or 0 if timeout, else -1
    */
    s32 getEvents(SEvent* iEvent, s32 iSize, u32 iTime);

    //EPOLL_CTL_ADD
    bool add(s32 fd, SEvent& iEvent);
    bool add(const net::CNetSocket& iSock, SEvent& iEvent);

    //EPOLL_CTL_MOD
    bool modify(s32 fd, SEvent& iEvent);
    bool modify(const net::CNetSocket& iSock, SEvent& iEvent);

    //EPOLL_CTL_DEL
    bool remove(s32 fd);
    bool remove(const net::CNetSocket& iSock);

    bool postEvent(SEvent& iEvent);

    net::CNetSocketPair& getSocketPair() {
        return mSocketPair;
    }

protected:
    s32 mEpollFD;
    net::CNetSocketPair mSocketPair; ///<send command to epoll
};

} //namespace app

#endif



#endif //APP_CEVENTPOLLER_H
