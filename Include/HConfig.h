#ifndef APP_HCONFIG_H
#define APP_HCONFIG_H

#define APP_USE_DATABASE

// Define the system
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#define APP_PLATFORM_WINDOWS
#define _IRR_WINDOWS_API_
#define APP_INLINE inline
#define APP_FORCE_INLINE __forceinline
#else
#define APP_PLATFORM_LINUX
#define _IRR_POSIX_API_
#define APP_INLINE inline
#define APP_FORCE_INLINE  __inline__ __attribute__((always_inline))
#endif


#if defined(_WIN64)  || defined(WIN64) || defined(__LP64__)  || defined(__amd64) || defined(__x86_64__)
#define APP_OS_64BIT
#define __IRR_HAS_S64
#else
#if defined(_WIN32) || defined(WIN32) || defined(__unix__)  || defined(__i386__)
#define APP_OS_32BIT
#endif
#endif



#if defined(_DEBUG)  || defined(DEBUG)
#define APP_DEBUG
#else
#define APP_RELEASE
#endif


//Define byte order
#ifndef APP_ENDIAN_BIG
#ifdef __BIG_ENDIAN__
#define APP_ENDIAN_BIG
#endif  //APP_ENDIAN_BIG

#ifndef APP_ENDIAN_BIG
#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MISPEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__) || defined(__powerpc__) || \
    defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#define APP_ENDIAN_BIG
#endif
#endif  //APP_ENDIAN_BIG

#endif  //Define byte order




#define _IRR_STATIC_LIB_

#if defined(APP_PLATFORM_WINDOWS)
#ifndef _IRR_STATIC_LIB_
#ifdef IRRLICHT_EXPORTS
#define IRRLICHT_API __declspec(dllexport)
#else
#define IRRLICHT_API __declspec(dllimport)
#endif // IRRLICHT_EXPORT
#else
#define IRRLICHT_API
#endif // _IRR_STATIC_LIB_

// Declare the calling convention.
#if defined(_STDCALL_SUPPORTED)
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV __cdecl
#endif // STDCALL_SUPPORTED

#endif //APP_PLATFORM_WINDOWS


#if defined(APP_PLATFORM_LINUX)
#ifndef _IRR_STATIC_LIB_
#ifdef IRRLICHT_EXPORTS
#define IRRLICHT_API
#else
#define IRRLICHT_API
#endif // IRRLICHT_EXPORT
#else
#define IRRLICHT_API
#endif // _IRR_STATIC_LIB_

// Declare the calling convention.
#if defined(_STDCALL_SUPPORTED)
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV
#endif // STDCALL_SUPPORTED

#endif //APP_PLATFORM_LINUX


//! define a break macro for debugging.
#include "assert.h"
#define APP_ASSERT( _CONDITION_ ) assert(_CONDITION_);


#if defined(UNICODE)
#define _IRR_WCHAR_FILESYSTEM
#endif

///Define for thread
///Define to desired default thread stack size  // Zero means use OS default
//#define APP_THREAD_STACK_SIZE 100
#ifndef APP_THREAD_STACK_SIZE
#define APP_THREAD_STACK_SIZE 0
#endif

///Define for thread
#if defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#define APP_HAVE_MUTEX_TIMEOUT
#endif


#define APP_GET_VALUE_POINTER(_POINTER_, _TYPE_, _ELEMENT_NAME_)   \
    ((_TYPE_*)(((s8*)((_TYPE_*)_POINTER_)) - ((size_t) &((_TYPE_*)0)->_ELEMENT_NAME_)))

#endif //APP_HCONFIG_H
