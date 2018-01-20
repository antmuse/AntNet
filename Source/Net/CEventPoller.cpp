#include "CEventPoller.h"
#include "CNetSocket.h"

#if defined(APP_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#endif


#if defined(APP_PLATFORM_WINDOWS)
namespace irr {

CEventPoller::CEventPoller() {
    mHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}


CEventPoller::~CEventPoller() {
    if(INVALID_HANDLE_VALUE != mHandle) {
        ::CloseHandle(mHandle);
        mHandle = INVALID_HANDLE_VALUE;
    }
}


s32 CEventPoller::getError() {
    return ::GetLastError();
}


bool CEventPoller::getEvent(SEvent& iEvent, u32 iTime) {
    APP_ASSERT(sizeof(u32) == sizeof(DWORD));
    APP_ASSERT(sizeof(iEvent.mKey) == sizeof(ULONG_PTR));

    if(TRUE == ::GetQueuedCompletionStatus(
        mHandle,
        (DWORD*) (&iEvent.mBytes),
        (PULONG_PTR) (&iEvent.mKey),
        (LPOVERLAPPED*) (&iEvent.mPointer),
        iTime
        )) {
        return true;
    }

    return false;
}


u32 CEventPoller::getEvents(SEvent* iEvent, u32 iSize, u32 iTime) {
    //u32 getoff = APP_GET_OFFSET(iEvent, mInternal);
    //u32 off = APP_OFFSET(OVERLAPPED_ENTRY, Internal);
    APP_ASSERT(sizeof(u32) == sizeof(ULONG));
    APP_ASSERT(sizeof(OVERLAPPED_ENTRY) == sizeof(SEvent));
    APP_ASSERT(APP_GET_OFFSET(iEvent, mBytes) == APP_OFFSET(OVERLAPPED_ENTRY, dwNumberOfBytesTransferred));
    APP_ASSERT(APP_GET_OFFSET(iEvent, mInternal) == APP_OFFSET(OVERLAPPED_ENTRY, Internal));
    APP_ASSERT(APP_GET_OFFSET(iEvent, mKey) == APP_OFFSET(OVERLAPPED_ENTRY, lpCompletionKey));
    APP_ASSERT(APP_GET_OFFSET(iEvent, mPointer) == APP_OFFSET(OVERLAPPED_ENTRY, lpOverlapped));

    u32 retSize = 0;
    if(TRUE == ::GetQueuedCompletionStatusEx(
        mHandle,
        (OVERLAPPED_ENTRY*) iEvent,
        iSize,
        (DWORD*) &retSize,
        iTime,
        FALSE)) {
        return retSize;
    }

    return 0;
}

bool CEventPoller::add(const net::CNetSocket& iSock, void* iKey) {
    return add((void*) iSock.getValue(), iKey);
}


bool CEventPoller::add(void* fd, void* key) {
    return (0 != ::CreateIoCompletionPort(fd,
        mHandle,
        (ULONG_PTR) key,
        0
        ));
}


bool CEventPoller::cancel(const net::CNetSocket& iSock, void* overlp) {
    if(FALSE == ::CancelIoEx((void*) iSock.getValue(), (LPOVERLAPPED) overlp)) {
        return (ERROR_NOT_FOUND == getError() || ERROR_OPERATION_ABORTED == getError());
    }
    return true;
}


//useless
//bool CEventPoller::getResult(void* fd, void* userPointer) {
//    DWORD bytes;
//    return TRUE == ::GetOverlappedResult(fd, (LPOVERLAPPED) userPointer, &bytes, FALSE);
//}


bool CEventPoller::postEvent(SEvent& iEvent) {
    return (TRUE == ::PostQueuedCompletionStatus(
        mHandle,
        iEvent.mBytes,
        (ULONG_PTR) iEvent.mKey,
        LPOVERLAPPED(iEvent.mPointer)
        ));
}


} //namespace irr


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)


namespace irr {

CEventPoller::CEventPoller() {
    mEpollFD = ::epoll_create(0x7ffffff);
}


CEventPoller::~CEventPoller() {
    ::close(mEpollFD);
}

s32 CEventPoller::getError() {
    return errno;
}

s32 CEventPoller::getEvent(SEvent& iEvent, u32 iTime) {
    return ::epoll_wait(mEpollFD, (epoll_event*) (&iEvent), 1, iTime);
}


s32 CEventPoller::getEvents(SEvent* iEvent, u32 iSize, u32 iTime) {
    APP_ASSERT(sizeof(*iEvent) == sizeof(epoll_event));
    APP_ASSERT(APP_GET_OFFSET(iEvent, mEvent) == APP_OFFSET(epoll_event, events));
    APP_ASSERT(APP_GET_OFFSET(iEvent, mData) == APP_OFFSET(epoll_event, data));

    return ::epoll_wait(mEpollFD, (epoll_event*) iEvent, iSize, iTime);
}

bool CEventPoller::add(const net::CNetSocket& iSock, SEvent& iEvent) {
    return add(iSock.getValue(), iEvent);
}

bool CEventPoller::add(s32 fd, SEvent& iEvent) {
    /*epoll_event ev;
    ev.events = 0;
    ev.data.ptr = iEvent.mData.mPointer;*/
    APP_ASSERT(sizeof(iEvent) == sizeof(epoll_event));
    //APP_ASSERT(APP_GET_OFFSET(&iEvent, mEvent) == APP_OFFSET(epoll_event, events));
    //APP_ASSERT(APP_GET_OFFSET(&iEvent, mData) == APP_OFFSET(epoll_event, data));

    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_ADD, fd, (epoll_event*) (&iEvent));
}

bool CEventPoller::remove(const net::CNetSocket& iSock) {
    return remove(iSock.getValue());
}


bool CEventPoller::remove(s32 fd) {
    //epoll_event iEvent;
    //iEvent.events = 0xffffffff;
    //iEvent.data.ptr = 0;
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_DEL, fd, 0);
}


bool CEventPoller::modify(const net::CNetSocket& iSock, SEvent& iEvent) {
    return modify(iSock.getValue(), iEvent);
}

bool CEventPoller::modify(s32 fd, SEvent& iEvent) {
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_MOD, fd, (epoll_event*) (&iEvent));
}


bool CEventPoller::postEvent(SEvent& iEvent) {
    return sizeof(iEvent.mData.mData32) == mSocket.sendAll((c8*) iEvent.mData.mData32, sizeof(iEvent.mData.mData32));
}


} //namespace irr

#endif
