#ifndef APP_MATH_H
#define APP_MATH_H

#include "HConfig.h"
#include <math.h>
#include <float.h>
#include <stdlib.h> // for abs() etc.
#include <limits.h> // For INT_MAX / UINT_MAX

#ifndef FLT_MAX
#define FLT_MAX 3.402823466E+38F
#endif

#ifndef FLT_MIN
#define FLT_MIN 1.17549435e-38F
#endif

namespace app {
namespace core {

//! Rounding error constant often used when comparing f32 values.
const f32 ROUNDING_ERROR_f32 = 0.000001f;
const f64 ROUNDING_ERROR_f64 = 0.00000001;

#ifdef PI // make sure we don't collide with a define
#undef PI
#endif

//! Constant for PI.
const f32 PI = 3.14159265359f;

//! Constant for reciprocal of PI.
const f32 RECIPROCAL_PI = 1.0f / PI;

//! Constant for half of PI.
const f32 HALF_PI = PI / 2.0f;

#ifdef PI64 // make sure we don't collide with a define
#undef PI64
#endif

//! Constant for 64bit PI.
const f64 PI64 = 3.1415926535897932384626433832795028841971693993751;

//! Constant for 64bit reciprocal of PI.
const f64 RECIPROCAL_PI64 = 1.0 / PI64;

//! 32bit Constant for converting from degrees to radians
const f32 DEGTORAD = PI / 180.0f;

//! 32bit constant for converting from radians to degrees (formally known as GRAD_PI)
const f32 RADTODEG = 180.0f / PI;

//! 64bit constant for converting from degrees to radians (formally known as GRAD_PI2)
const f64 DEGTORAD64 = PI64 / 180.0;

//! 64bit constant for converting from radians to degrees
const f64 RADTODEG64 = 180.0 / PI64;

//! Utility function to convert a radian value to degrees
/** Provided as it can be clearer to write radToDeg(X) than RADTODEG * X
\param radians	The radians value to convert to degrees.
*/
inline f32 radToDeg(f32 radians) {
    return RADTODEG * radians;
}

//! Utility function to convert a radian value to degrees
/** Provided as it can be clearer to write radToDeg(X) than RADTODEG * X
\param radians	The radians value to convert to degrees.
*/
inline f64 radToDeg(f64 radians) {
    return RADTODEG64 * radians;
}

//! Utility function to convert a degrees value to radians
/** Provided as it can be clearer to write degToRad(X) than DEGTORAD * X
\param degrees	The degrees value to convert to radians.
*/
inline f32 degToRad(f32 degrees) {
    return DEGTORAD * degrees;
}

//! Utility function to convert a degrees value to radians
/** Provided as it can be clearer to write degToRad(X) than DEGTORAD * X
\param degrees	The degrees value to convert to radians.
*/
inline f64 degToRad(f64 degrees) {
    return DEGTORAD64 * degrees;
}

//! returns minimum of two values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& min_(const T& a, const T& b) {
    return a < b ? a : b;
}

//! returns minimum of three values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& min_(const T& a, const T& b, const T& c) {
    return a < b ? min_(a, c) : min_(b, c);
}

//! returns maximum of two values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& max_(const T& a, const T& b) {
    return a < b ? b : a;
}

//! returns maximum of three values. Own implementation to get rid of the STL (VS6 problems)
template<class T>
inline const T& max_(const T& a, const T& b, const T& c) {
    return a < b ? max_(b, c) : max_(a, c);
}

//! returns abs of two values. Own implementation to get rid of STL (VS6 problems)
template<class T>
inline T abs_(const T& a) {
    return a < (T)0 ? -a : a;
}

//! returns linear interpolation of a and b with ratio t
//! \return: a if t==0, b if t==1, and the linear interpolation else
template<class T>
inline T lerp(const T& a, const T& b, const f32 t) {
    return (T)(a*(1.f - t)) + (b*t);
}

//! clamps a value between low and high
template <class T>
inline const T clamp(const T& value, const T& low, const T& high) {
    return min_(max_(value, low), high);
}

//! swaps the content of the passed parameters
// Note: We use the same trick as boost and use two template arguments to
// avoid ambiguity when swapping objects of an Irrlicht type that has not
// it's own swap overload. Otherwise we get conflicts with some compilers
// in combination with stl.
template <class T1, class T2>
inline void swap(T1& a, T2& b) {
    T1 c(a);
    a = b;
    b = c;
}

//! returns if a equals b, taking possible rounding errors into account
inline bool equals(const f64 a, const f64 b, const f64 tolerance = ROUNDING_ERROR_f64) {
    return (a + tolerance >= b) && (a - tolerance <= b);
}

//! returns if a equals b, taking possible rounding errors into account
inline bool equals(const f32 a, const f32 b, const f32 tolerance = ROUNDING_ERROR_f32) {
    return (a + tolerance >= b) && (a - tolerance <= b);
}


//! returns if a equals zero, taking rounding errors into account
inline bool iszero(const f64 a, const f64 tolerance = ROUNDING_ERROR_f64) {
    return fabs(a) <= tolerance;
}

//! returns if a equals zero, taking rounding errors into account
inline bool iszero(const f32 a, const f32 tolerance = ROUNDING_ERROR_f32) {
    return fabsf(a) <= tolerance;
}

//! returns if a equals not zero, taking rounding errors into account
inline bool isnotzero(const f32 a, const f32 tolerance = ROUNDING_ERROR_f32) {
    return fabsf(a) > tolerance;
}

//! returns if a equals zero, taking rounding errors into account
inline bool iszero(const s32 a, const s32 tolerance = 0) {
    return (a & 0x7ffffff) <= tolerance;
}

//! returns if a equals zero, taking rounding errors into account
inline bool iszero(const u32 a, const u32 tolerance = 0) {
    return a <= tolerance;
}

//! returns if a equals zero, taking rounding errors into account
inline bool iszero(const s64 a, const s64 tolerance = 0) {
    return abs_(a) <= tolerance;
}

inline s32 s32_min(s32 a, s32 b) {
    const s32 mask = (a - b) >> 31;
    return (a & mask) | (b & ~mask);
}

inline s32 s32_max(s32 a, s32 b) {
    const s32 mask = (a - b) >> 31;
    return (b & mask) | (a & ~mask);
}

inline s32 s32_clamp(s32 value, s32 low, s32 high) {
    return s32_min(s32_max(value, low), high);
}

//! conditional set based on mask and arithmetic shift
APP_FORCE_INLINE u32 if_c_a_else_b(const s32 condition, const u32 a, const u32 b) {
    return ((-condition >> 31) & (a ^ b)) ^ b;
}

//! conditional set based on mask and arithmetic shift
APP_FORCE_INLINE u16 if_c_a_else_b(const s16 condition, const u16 a, const u16 b) {
    return ((-condition >> 15) & (a ^ b)) ^ b;
}

//! conditional set based on mask and arithmetic shift
APP_FORCE_INLINE u32 if_c_a_else_0(const s32 condition, const u32 a) {
    return (-condition >> 31) & a;
}

/*
  if (condition) state |= m; else state &= ~m;
*/
APP_FORCE_INLINE void setbit_cond(u32& state, s32 condition, u32 mask) {
    // 0, or any postive to mask
    //s32 conmask = -condition >> 31;
    state ^= ((-condition >> 31) ^ state) & mask;
}


inline f32 f32_max3(const f32 a, const f32 b, const f32 c) {
    return a > b ? (a > c ? a : c) : (b > c ? b : c);
}

inline f32 f32_min3(const f32 a, const f32 b, const f32 c) {
    return a < b ? (a < c ? a : c) : (b < c ? b : c);
}



} // end namespace core
} // end namespace app

#endif

