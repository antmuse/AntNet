#include "HConfig.h"

namespace app {
namespace core {

/**
 * @brief usage:
 *   wchar_t dest[8];
 *   size_t strlen = AppUTF8ToWchar("src",dest,sizeof(dest));
 *
 * @param src input str
 * @param dest output cache
 * @param osize sizeof output cache
 * @return length of dest wstring.
 */
usz AppUTF8ToWchar(const s8* src, wchar_t* dest, const usz osize);

/**
 * @brief usage:
 *   wchar_t dest[8];
 *   size_t strlen = AppUTF8ToWchar("src",dest,sizeof(dest));
 *
 * @param src input str
 * @param dest output cache
 * @param osize sizeof output cache
 * @return length of dest string.
 */
usz AppWcharToUTF8(const wchar_t* src, s8* dest, const usz osize);

/**
* @see AppUTF8ToWchar()
*/
usz AppUTF8ToUCS4(const s8* src, u32* dest, usz osize);

/**
* @see AppUTF8ToWchar()
*/
usz AppUTF8ToUCS2(const s8* src, u16* dest, usz osize);

/**
* @see AppWcharToUTF8()
*/
usz AppUCS4ToUTF8(const u32* src, s8* dest, usz osize);

/**
* @see AppWcharToUTF8()
*/
usz AppUCS2ToUTF8(const u16* src, s8* dest, usz osize);

} // end namespace core
} // end namespace app

