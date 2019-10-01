#ifndef APP_HCONFIG_H
#define APP_HCONFIG_H

#define APP_USE_DATABASE

// Define the system
#if !defined( APP_PLATFORM_WINDOWS )
#if defined(_WIN64)  || defined(WIN64) || defined(_WIN32) || defined(WIN32)
#define APP_PLATFORM_WINDOWS
#define APP_INLINE inline
#define APP_FORCE_INLINE __forceinline
#define APP_ALIGN(N) __declspec(align(N))
#endif
#endif


#if !defined( APP_PLATFORM_LINUX )
#if (defined(_UNIX) || defined(__linux)) && !defined(ANDROID_NDK) && !defined(__ANDROID__)
#define APP_PLATFORM_LINUX
#define APP_INLINE inline
#define APP_FORCE_INLINE inline
#define APP_ALIGN(N) __attribute__((__aligned__((N))))
#endif
#endif


#if !defined( APP_PLATFORM_ANDROID )
#if defined(ANDROID_NDK) || defined(__ANDROID__)
#define APP_PLATFORM_ANDROID
#define APP_INLINE inline
#define APP_FORCE_INLINE inline
#define APP_ALIGN(N) __attribute__((__aligned__((N))))
// On Android platform, i use irrlicht1.8
#define APP_USE_IRRLICHT_OLD
#endif
#endif


#if defined(_WIN64)  || defined(WIN64) || defined(__LP64__)  || defined(__amd64) || defined(__x86_64__)
#define APP_OS_64BIT
#define __IRR_HAS_S64
#else
#if defined(_WIN32) || defined(WIN32) || defined(__unix__)  || defined(__i386__)
#define APP_OS_32BIT
#define __IRR_HAS_S64
#endif
#endif



#if defined(_DEBUG) || defined(DEBUG)
#define APP_DEBUG

//! define a break macro for debugging.
#include "assert.h"
#define APP_ASSERT( _CONDITION_ ) assert(_CONDITION_);

#else
#define APP_RELEASE
#define APP_ASSERT(_CND_)
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

#define APP_SWAP32(X) ((((X)&0x000000FF)<<24) | (((X)&0xFF000000) >> 24) | (((X)&0x0000FF00) << 8) | (((X) &0x00FF0000) >> 8))
#define APP_SWAP16(X) ((((X)&0x00FF)<<8) | (((X)&0xFF00)>>8))

#define APP_OFFSET(_TYPE_, _ELEMENT_NAME_) ((size_t)(&(((_TYPE_*)0)->_ELEMENT_NAME_)))
#define APP_GET_OFFSET(_POINTER_, _ELEMENT_NAME_) (((size_t)(&(_POINTER_->_ELEMENT_NAME_))) - ((size_t)(_POINTER_)))
#define APP_GET_VALUE_POINTER(_POINTER_, _TYPE_, _ELEMENT_NAME_)   \
    ((_TYPE_*)(((s8*)((_TYPE_*)_POINTER_)) - ((size_t) &((_TYPE_*)0)->_ELEMENT_NAME_)))


//d=整数,a=对齐单位(必须是2的幂次方)
#define APP_ALIGN_DATA(d, a)         (((d) + ((a) - 1)) & ~((a) - 1))
//p=指针,a=对齐单位(必须是2的幂次方)
#define APP_ALIGN_POINTER(p, a) ((char*) (((unsigned long long) (p) + ((a) - 1ULL)) & ~((a) -1ULL)))


#define APP_COMPILE_WITH_CONSOLE_LOG_RECEIVER


#endif //APP_HCONFIG_H
