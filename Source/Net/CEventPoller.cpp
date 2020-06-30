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
namespace app {

CEventPoller::CEventPoller() {
    APP_ASSERT(sizeof(SEvent::mKey) == sizeof(ULONG_PTR));
    APP_ASSERT(sizeof(OVERLAPPED_ENTRY) == sizeof(SEvent));
    APP_ASSERT(APP_OFFSET(SEvent, mBytes) == APP_OFFSET(OVERLAPPED_ENTRY, dwNumberOfBytesTransferred));
    APP_ASSERT(APP_OFFSET(SEvent, mInternal) == APP_OFFSET(OVERLAPPED_ENTRY, Internal));
    APP_ASSERT(APP_OFFSET(SEvent, mKey) == APP_OFFSET(OVERLAPPED_ENTRY, lpCompletionKey));
    APP_ASSERT(APP_OFFSET(SEvent, mPointer) == APP_OFFSET(OVERLAPPED_ENTRY, lpOverlapped));

    mHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}


CEventPoller::~CEventPoller() {
    close();
}

void CEventPoller::close() {
    if(INVALID_HANDLE_VALUE != mHandle) {
        ::CloseHandle(mHandle);
        mHandle = INVALID_HANDLE_VALUE;
    }
}

s32 CEventPoller::getError() {
    return ::GetLastError();
}


s32 CEventPoller::getEvent(SEvent& iEvent, u32 iTime) {
    if(TRUE == ::GetQueuedCompletionStatus(
        mHandle,
        (DWORD*)(&iEvent.mBytes),
        (PULONG_PTR)(&iEvent.mKey),
        (LPOVERLAPPED*)(&iEvent.mPointer),
        iTime
        )) {
        return 1;
    }

    return -1;
}


s32 CEventPoller::getEvents(SEvent* iEvent, s32 iSize, u32 iTime) {
    //APP_ASSERT(APP_GET_OFFSET(iEvent, mBytes) == APP_OFFSET(OVERLAPPED_ENTRY, dwNumberOfBytesTransferred));
    //APP_ASSERT(APP_GET_OFFSET(iEvent, mInternal) == APP_OFFSET(OVERLAPPED_ENTRY, Internal));
    //APP_ASSERT(APP_GET_OFFSET(iEvent, mKey) == APP_OFFSET(OVERLAPPED_ENTRY, lpCompletionKey));
    //APP_ASSERT(APP_GET_OFFSET(iEvent, mPointer) == APP_OFFSET(OVERLAPPED_ENTRY, lpOverlapped));

    u32 retSize = 0;
    if(TRUE == ::GetQueuedCompletionStatusEx(
        mHandle,
        (OVERLAPPED_ENTRY*)iEvent,
        iSize,
        (DWORD*)& retSize,
        iTime,
        FALSE)) {
        return retSize;
    }

    return -1;
}

bool CEventPoller::add(const net::CNetSocket& iSock, void* iKey) {
    return add((void*)iSock.getValue(), iKey);
}


bool CEventPoller::add(void* fd, void* key) {
    return (0 != ::CreateIoCompletionPort(fd, mHandle, (ULONG_PTR)key, 0));
}

bool CEventPoller::cancelIO(void* handle) {
    if(FALSE == ::CancelIo(handle)) {
        return (ERROR_NOT_FOUND == getError() || ERROR_OPERATION_ABORTED == getError());
    }
    return true;
}

bool CEventPoller::hasOverlappedIoCompleted(void* overlp) {
    return (overlp && HasOverlappedIoCompleted(reinterpret_cast<LPOVERLAPPED>(overlp)));
}

bool CEventPoller::cancelIO(void* handle, void* overlp) {
    if(FALSE == ::CancelIoEx(handle, reinterpret_cast<LPOVERLAPPED>(overlp))) {
        return (ERROR_NOT_FOUND == getError() || ERROR_OPERATION_ABORTED == getError());
    }
    return true;
}

bool CEventPoller::cancelIO(const net::CNetSocket& sock) {
    return cancelIO(reinterpret_cast<void*>(sock.getValue()));
}

bool CEventPoller::cancelIO(const net::CNetSocket& sock, void* overlp) {
    return cancelIO(reinterpret_cast<void*>(sock.getValue()), overlp);
}

bool CEventPoller::getResult(void* fd, void* userPointer, u32* bytes, u32 wait) {
    return TRUE == ::GetOverlappedResult(fd,
        reinterpret_cast<LPOVERLAPPED>(userPointer),
        reinterpret_cast<LPDWORD>(bytes),
        wait);
}


bool CEventPoller::postEvent(SEvent& iEvent) {
    return (TRUE == ::PostQueuedCompletionStatus(
        mHandle,
        iEvent.mBytes,
        (ULONG_PTR)iEvent.mKey,
        LPOVERLAPPED(iEvent.mPointer)
        ));
}


} //namespace app


#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)


namespace app {

CEventPoller::CEventPoller() {
    APP_ASSERT(sizeof(SEvent) == sizeof(epoll_event));
    APP_ASSERT(APP_OFFSET(SEvent, mEvent) == APP_OFFSET(epoll_event, events));
    APP_ASSERT(APP_OFFSET(SEvent, mData) == APP_OFFSET(epoll_event, data));

    mEpollFD = ::epoll_create(0x7ffffff);
    bool ret = mSocketPair.open();
    APP_ASSERT(ret);
}


CEventPoller::~CEventPoller() {
    mSocketPair.close();
    ::close(mEpollFD);
}

s32 CEventPoller::getError() {
    return errno;
}

s32 CEventPoller::getEvent(SEvent& iEvent, u32 iTime) {
    return ::epoll_wait(mEpollFD, (epoll_event*)(&iEvent), 1, iTime);
}


s32 CEventPoller::getEvents(SEvent* iEvent, s32 iSize, u32 iTime) {
    s32 ret = ::epoll_wait(mEpollFD, (epoll_event*)iEvent, iSize, iTime);
    return (ret > 0 ? ret : 0);
}

bool CEventPoller::add(const net::CNetSocket& iSock, SEvent& iEvent) {
    return add(iSock.getValue(), iEvent);
}

bool CEventPoller::add(s32 fd, SEvent& iEvent) {
    /*epoll_event ev;
    ev.events = 0;
    ev.data.ptr = iEvent.mData.mPointer;*/
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_ADD, fd, (epoll_event*)(&iEvent));
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
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_MOD, fd, (epoll_event*)(&iEvent));
}


bool CEventPoller::postEvent(SEvent& iEvent) {
    return sizeof(iEvent.mEvent) == mSocketPair.getSocketB().sendAll(&iEvent.mEvent, sizeof(iEvent.mEvent));
}


} //namespace app

#endif
