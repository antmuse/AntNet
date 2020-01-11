/***********************************************************************************
* @file HConfig.h
* @brief The main configuration of this project.
*
* All source head files must include this file firstly.
* Copyright (c) 2008-2017 Antmuse
* This file is part of my "NeatSpark" task.
* For conditions of distribution and use, see copyright notice in HConfig.h
* @author   antmuse@live.cn
* @version   0.3.4.70
* @date  2012-10-04, PM2:35
************************************************************************************/

#ifndef ANTMUSE_HCONFIG_H
#define	ANTMUSE_HCONFIG_H

////////////////////////////////////////////////////////////////////////////////

//Configure: Version
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 3
#define APP_VERSION_MICRO 5
#define APP_VERSION_MINI  0
#define APP_VERSION_NAME  "0.3.5.0"
#define APP_VERSION_ID    3500

//Configure: Application Defender
#define APP_DEFENDER


//Configure: Debug or Release
#if !defined( APP_DEBUG )
#if defined(DEBUG) || defined(_DEBUG)
#define APP_DEBUG
#else
#define APP_RELEASE
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

//Configure: Platform
#if !defined( APP_PLATFORM_WINDOWS )
#if defined(_WIN64)  || defined(WIN64) || defined(_WIN32) || defined(WIN32)
#define APP_PLATFORM_WINDOWS
#define APP_INLINE inline
#define APP_FORCE_INLINE __forceinline
#define APP_ALIGN(N) __declspec(align(N))

#if defined(_UNICODE) || defined(UNICODE)
#define _IRR_WCHAR_FILESYSTEM
#endif

#endif
#endif


#if !defined( APP_PLATFORM_LINUX )
#if (defined(_UNIX) || defined(__linux)) && !defined(ANDROID_NDK) && !defined(__ANDROID__)
#define APP_PLATFORM_LINUX
#define APP_INLINE inline
#define APP_FORCE_INLINE __attribute__((always_inline)) inline
#define APP_ALIGN(N) __attribute__((__aligned__((N))))
#endif
#endif


#if !defined( APP_PLATFORM_ANDROID )
#if defined(ANDROID_NDK) || defined(__ANDROID__)
#define APP_PLATFORM_ANDROID
#define APP_INLINE inline
#define APP_FORCE_INLINE __attribute__((always_inline)) inline
#define APP_ALIGN(N) __attribute__((__aligned__((N))))
// On Android platform, i use irrlicht1.8
#define APP_USE_IRRLICHT_OLD
#endif
#endif


#if defined(_WIN64)  || defined(WIN64) || defined(__LP64__)  || defined(__amd64) || defined(__x86_64__) || defined(__aarch64__)
#define APP_OS_64BIT
#else
#if defined(_WIN32) || defined(WIN32) || defined(__unix__)  || defined(__i386__)
#define APP_OS_32BIT
#endif
#endif


//Define byte order
#ifndef APP_ENDIAN_BIG
#ifdef __BIG_ENDIAN__
#define APP_ENDIAN_BIG
#endif


#ifndef APP_ENDIAN_BIG
#if defined(__hppa__) || \
	defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
	(defined(__MIPS__) && defined(__MISPEB__)) || \
	defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
	defined(__sparc__) || defined(__powerpc__) || \
	defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#define APP_ENDIAN_BIG
#endif
#endif
#endif  //Define byte order


#if !defined( APP_PLATFORM_MAC ) || !defined( APP_PLATFORM_IOS )
#if defined( __APPLE__ ) || defined( __APPLE_CC__ ) || defined( MACOSX )
#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 30000 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 30000
#define APP_PLATFORM_IOS
#else
#define APP_PLATFORM_MAC
#endif
#endif
#endif


#ifndef APP_COMPILIER_GCC
#if defined(__GNUC__) || defined(__GCCXML__)
#define APP_COMPILIER_GCC
#endif
#endif

#ifndef APP_COMPILIER_MSVC
#if defined(_MSC_VER)
#if _MSC_VER >= 1400
#define APP_COMPILIER_MSVC
#else
#error This version of the MSVC compilier is not supported
#endif
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
// Define to desired default thread stack size  // Zero means use OS default
//#define APP_THREAD_STACK_SIZE 100
#ifndef APP_THREAD_STACK_SIZE
#define APP_THREAD_STACK_SIZE 0
#endif


#ifdef APP_DEBUG
#include <assert.h>
#define APP_ASSERT(iName)  assert(iName)
#define APP_PROFILE(iName)
#else
#define APP_ASSERT(iName)
#define APP_PROFILE(iName)
#endif


////////////////////////////////////////////////////////////////////////////////
//Configure: API
#define APP_API_C extern "C"

#define APP_STATIC_LIB
#ifdef APP_NO_STATIC_LIB
#undef APP_STATIC_LIB
#endif


#ifdef APP_STATIC_LIB
#define APP_API
#else
#ifdef APP_EXPORTS
// We are building this library
#define APP_API __declspec(dllexport)
#else
// We are using this library
#define APP_API __declspec(dllimport)
#endif
#endif


#define APP_SWAP32(X) ((((X)&0x000000FF)<<24) | (((X)&0xFF000000) >> 24) | (((X)&0x0000FF00) << 8) | (((X) &0x00FF0000) >> 8))
#define APP_SWAP16(X) ((((X)&0x00FF)<<8) | (((X)&0xFF00)>>8))
#define APP_SWAP64(X) ((((X)&0x00000000000000FFULL)<<56) | (((X)&0xFF00000000000000ULL)>>56)  \
                    | (((X)&0x000000000000FF00ULL)<<40) | (((X)&0x00FF000000000000ULL)>>40)  \
                    | (((X)&0x0000000000FF0000ULL)<<24) | (((X)&0x0000FF0000000000ULL)>>24)  \
                    | (((X)&0x00000000FF000000ULL)<< 8) | (((X)&0x000000FF00000000ULL)>> 8))


#define APP_OFFSET(_TYPE_, _ELEMENT_NAME_) ((size_t)(&(((_TYPE_*)0)->_ELEMENT_NAME_)))
#define APP_GET_OFFSET(_POINTER_, _ELEMENT_NAME_) (((size_t)(&(_POINTER_->_ELEMENT_NAME_))) - ((size_t)(_POINTER_)))
#define APP_GET_VALUE_POINTER(_POINTER_, _TYPE_, _ELEMENT_NAME_)   \
	((_TYPE_*)(((char*)((_TYPE_*)_POINTER_)) - ((size_t) &((_TYPE_*)0)->_ELEMENT_NAME_)))


//d=整数,a=对齐单位(必须是2的幂次方)
#define APP_ALIGN_DATA(d, a)         (((d) + ((a) - 1)) & ~((a) - 1))
//p=指针,a=对齐单位(必须是2的幂次方)
#define APP_ALIGN_POINTER(p, a) ((char*) (((unsigned long long) (p) + ((a) - 1ULL)) & ~((a) -1ULL)))


// Declare the calling convention.
#if defined(APP_STDCALL_SUPPORTED)
#define APP_CALL __stdcall
#else
#define APP_CALL __cdecl
#endif // STDCALL_SUPPORTED
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#endif	/* ANTMUSE_HCONFIG_H */