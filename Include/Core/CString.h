#ifndef APP_TSTRING_H
#define APP_TSTRING_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "HConfig.h"
#include "TAllocator.h"
#include "AppMath.h"


namespace app {
namespace core {

static inline u32 App2Lower(u32 x) {
    return x >= 'A' && x <= 'Z' ? x + 0x20 : x;
}

static inline u32 App2Upper(u32 x) {
    return x >= 'a' && x <= 'z' ? x + ('A' - 'a') : x;
}


/**
* @brief Very simple string class with some useful features.
*/
template <typename T, typename TAlloc = TAllocator<T> >
class TString {
public:

    //! Default constructor
    TString()
        : mAllocated(8), mUsed(1) {
        mBuffer = mAllocator.allocate(8); // new T[8];
        mBuffer[0] = 0;
    }


    //! Constructor
    TString(const TString<T, TAlloc>& other)
        : mBuffer(0), mAllocated(0), mUsed(0) {
        *this = other;
    }

    //! Constructor from other TString types
    template <class B, class A>
    TString(const TString<B, A>& other)
        : mBuffer(0), mAllocated(0), mUsed(0) {
        *this = other;
    }

    explicit TString(const f32 number)
        : mBuffer(0), mAllocated(0), mUsed(0) {
        s8 tmpbuf[255];
        snprintf(tmpbuf, 255, "%0.6f", number);
        *this = tmpbuf;
    }

    //! Constructs a TString from a float
    explicit TString(const f64 number)
        : mBuffer(0), mAllocated(0), mUsed(0) {
        s8 tmpbuf[255];
        snprintf(tmpbuf, 255, "%0.6f", number);
        *this = tmpbuf;
    }


    //! Constructs a TString from an int
    explicit TString(s32 number)
        : mBuffer(0), mAllocated(0), mUsed(0) {
        // store if negative and make positive

        bool negative = false;
        if(number < 0) {
            number *= -1;
            negative = true;
        }

        // temporary buffer for 16 numbers

        s8 tmpbuf[16] = {0};
        u32 idx = 15;

        // special case '0'

        if(!number) {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers

        while(number && idx) {
            --idx;
            tmpbuf[idx] = (s8)('0' + (number % 10));
            number /= 10;
        }

        // add sign

        if(negative) {
            --idx;
            tmpbuf[idx] = '-';
        }

        *this = &tmpbuf[idx];
    }


    //! Constructs a TString from an unsigned int
    explicit TString(u32 number)
        : mBuffer(0), mAllocated(0), mUsed(0) {
        // temporary buffer for 16 numbers

        s8 tmpbuf[16] = {0};
        u32 idx = 15;

        // special case '0'

        if(!number) {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers

        while(number && idx) {
            --idx;
            tmpbuf[idx] = (s8)('0' + (number % 10));
            number /= 10;
        }

        *this = &tmpbuf[idx];
    }


    //! Constructor for copying a TString from a pointer with a given length
    template <class B>
    TString(const B* const c, u32 length) : mBuffer(0), mAllocated(0), mUsed(0) {
        if(!c) {
            // correctly init the TString to an empty one
            *this = "";
            return;
        }

        mAllocated = mUsed = length + 1;
        mBuffer = mAllocator.allocate(mUsed); // new T[mUsed];

        for(u32 l = 0; l < length; ++l) {
            mBuffer[l] = (T)c[l];
        }
        mBuffer[length] = 0;
    }


    //! Constructor for unicode and ascii strings
    template <class B>
    TString(const B* const c) : mBuffer(0), mAllocated(0), mUsed(0) {
        *this = c;
    }

    //TString(const T* const c) : mBuffer(0), mAllocated(0), mUsed(0) {
    //    *this = c;
    //}

    //! Destructor
    ~TString() {
        mAllocator.deallocate(mBuffer); // delete [] mBuffer;
    }


    //! Assignment operator
    TString<T, TAlloc>& operator=(const TString<T, TAlloc>& other) {
        if(this == &other) {
            return *this;
        }
        mUsed = other.size() + 1;
        if(mUsed > mAllocated) {
            mAllocator.deallocate(mBuffer); // delete [] mBuffer;
            mAllocated = mUsed;
            mBuffer = mAllocator.allocate(mUsed); //new T[mUsed];
        }

        const T* p = other.c_str();
        for(u32 i = 0; i < mUsed; ++i, ++p) {
            mBuffer[i] = *p;
        }
        return *this;
    }

    //! Assignment operator for other TString types
    template <class B, class A>
    TString<T, TAlloc>& operator=(const TString<B, A>& other) {
        *this = other.c_str();
        return *this;
    }


    //! Assignment operator for strings, ascii and unicode
    template <class B>
    TString<T, TAlloc>& operator=(const B* const c) {
        if(!c) {
            if(!mBuffer) {
                mBuffer = mAllocator.allocate(1); //new T[1];
                mAllocated = 1;
            }
            mUsed = 1;
            mBuffer[0] = 0x0;
            return *this;
        }

        if((void*)c == (void*)mBuffer) {
            return *this;
        }
        u32 len = 0;
        const B* p = c;
        do {
            ++len;
        } while(*p++);

        // we'll keep the old TString for a while, because the new
        // TString could be a part of the current TString.
        T* oldArray = mBuffer;

        mUsed = len;
        if(mUsed > mAllocated) {
            mAllocated = mUsed;
            mBuffer = mAllocator.allocate(mUsed); //new T[mUsed];
        }

        for(u32 l = 0; l < len; ++l) {
            mBuffer[l] = (T)c[l];
        }
        if(oldArray != mBuffer) {
            mAllocator.deallocate(oldArray); // delete [] oldArray;
        }
        return *this;
    }


    //! Append operator for other strings
    TString<T, TAlloc> operator+(const TString<T, TAlloc>& other) const {
        TString<T, TAlloc> str(*this);
        str.append(other);
        return str;
    }


    //! Append operator for strings, ascii and unicode
    template <class B>
    TString<T, TAlloc> operator+(const B* const c) const {
        TString<T, TAlloc> str(*this);
        str.append(c);
        return str;
    }


    //! Direct access operator
    T& operator[](const u32 index) {
        APP_ASSERT(index < mUsed); // bad index
        return mBuffer[index];
    }


    //! Direct access operator
    const T& operator[](const u32 index) const {
        APP_ASSERT(index < mUsed); // bad index
        return mBuffer[index];
    }


    //! Equality operator
    bool operator==(const T* const str) const {
        if(!str) {
            return false;
        }
        u32 i;
        for(i = 0; mBuffer[i] && str[i]; ++i) {
            if(mBuffer[i] != str[i])
                return false;
        }
        return (!mBuffer[i] && !str[i]);
    }


    //! Equality operator
    bool operator==(const TString<T, TAlloc>& other) const {
        for(u32 i = 0; mBuffer[i] && other.mBuffer[i]; ++i) {
            if(mBuffer[i] != other.mBuffer[i])
                return false;
        }
        return mUsed == other.mUsed;
    }


    //! Is smaller comparator
    bool operator<(const TString<T, TAlloc>& other) const {
        for(u32 i = 0; mBuffer[i] && other.mBuffer[i]; ++i) {
            const s32 diff = mBuffer[i] - other.mBuffer[i];
            if(diff)
                return (diff < 0);
        }
        return (mUsed < other.mUsed);
    }


    //! Inequality operator
    bool operator!=(const T* const str) const {
        return !(*this == str);
    }


    //! Inequality operator
    bool operator!=(const TString<T, TAlloc>& other) const {
        return !(*this == other);
    }


    /**
    * @return Length of the TString's content in characters, excluding the trailing NUL. */
    u32 size() const {
        return mUsed - 1;
    }

    //! Informs if the TString is empty or not.
    //! \return True if the TString is empty, false if not.
    bool empty() const {
        return (size() == 0);
    }

    //! Returns character TString
    /** \return pointer to C-style NUL terminated TString. */
    const T* c_str() const {
        return mBuffer;
    }


    //! Makes the TString lower case.
    TString<T, TAlloc>& makeLower() {
        for(u32 i = 0; mBuffer[i]; ++i) {
            mBuffer[i] = App2Lower(mBuffer[i]);
        }
        return *this;
    }


    //! Makes the TString upper case.
    TString<T, TAlloc>& makeUpper() {
        for(u32 i = 0; mBuffer[i]; ++i) {
            mBuffer[i] = App2Upper(mBuffer[i]);
        }
        return *this;
    }


    //! Compares the strings ignoring case.
    /** \param other: Other TString to compare.
    \return True if the strings are equal ignoring case. */
    bool equals_ignore_case(const TString<T, TAlloc>& other) const {
        for(u32 i = 0; mBuffer[i] && other[i]; ++i) {
            if(App2Lower(mBuffer[i]) != App2Lower(other[i]))
                return false;
        }
        return mUsed == other.mUsed;
    }

    //! Compares the strings ignoring case.
    /** \param other: Other TString to compare.
        \param sourcePos: where to start to compare in the TString
    \return True if the strings are equal ignoring case. */
    bool equals_substring_ignore_case(const TString<T, TAlloc>& other, const s32 sourcePos = 0) const {
        if((u32)sourcePos >= mUsed)
            return false;

        u32 i;
        for(i = 0; mBuffer[sourcePos + i] && other[i]; ++i) {
            if(App2Lower(mBuffer[sourcePos + i]) != App2Lower(other[i]))
                return false;
        }
        return mBuffer[sourcePos + i] == 0 && other[i] == 0;
    }


    //! Compares the strings ignoring case.
    /** \param other: Other TString to compare.
    \return True if this TString is smaller ignoring case. */
    bool lower_ignore_case(const TString<T, TAlloc>& other) const {
        for(u32 i = 0; mBuffer[i] && other.mBuffer[i]; ++i) {
            s32 diff = (s32)App2Lower(mBuffer[i]) - (s32)App2Lower(other.mBuffer[i]);
            if(diff)
                return diff < 0;
        }
        return mUsed < other.mUsed;
    }


    //! compares the first n characters of the strings
    /** \param other Other TString to compare.
    \param n Number of characters to compare
    \return True if the n first characters of both strings are equal. */
    bool equalsn(const TString<T, TAlloc>& other, u32 n) const {
        u32 i;
        for(i = 0; mBuffer[i] && other[i] && i < n; ++i) {
            if(mBuffer[i] != other[i])
                return false;
        }
        // if one (or both) of the strings was smaller then they
        // are only equal if they have the same length
        return (i == n) || (mUsed == other.mUsed);
    }


    //! compares the first n characters of the strings
    /** \param str Other TString to compare.
    \param n Number of characters to compare
    \return True if the n first characters of both strings are equal. */
    bool equalsn(const T* const str, u32 n) const {
        if(!str) {
            return false;
        }
        u32 i;
        for(i = 0; mBuffer[i] && str[i] && i < n; ++i) {
            if(mBuffer[i] != str[i])
                return false;
        }
        // if one (or both) of the strings was smaller then they
        // are only equal if they have the same length
        return (i == n) || (mBuffer[i] == 0 && str[i] == 0);
    }


    //! Appends a character to this TString
    /** \param character: Character to append. */
    TString<T, TAlloc>& append(T character) {
        if(mUsed + 1 > mAllocated) {
            reallocate(mUsed + 1);
        }
        ++mUsed;
        mBuffer[mUsed - 2] = character;
        mBuffer[mUsed - 1] = 0;
        return *this;
    }


    void setUsed(u32 val) {
        mUsed = val + 1;
        if(mUsed > mAllocated) {
            reallocate(mUsed);
        }
        mBuffer[mUsed - 1] = 0;
    }


    //! Appends a char TString to this TString
    /** \param other: Char TString to append. */
    /** \param length: The length of the TString to append. */
    TString<T, TAlloc>& append(const T* const other, u32 len = 0xFFFFFFFF) {
        if(!other) {
            return *this;
        }
        if(0xFFFFFFFF == len) {
            const T* p = other;
            while(*p++) { }
            len = (u32)(p - other) - 1;
        }
        if(mUsed + len > mAllocated) {
            reallocate(mUsed + len);
        }
        --mUsed;
        for(u32 i = 0; i < len; ++i) {
            mBuffer[mUsed + i] = other[i];
        }
        mUsed += len;
        mBuffer[mUsed++] = 0;
        return *this;
    }


    //! Appends a TString to this TString
    /** \param other: String to append. */
    TString<T, TAlloc>& append(const TString<T, TAlloc>& other) {
        if(other.size() == 0) {
            return *this;
        }
        return append(other.mBuffer, other.size());
    }


    //! Appends a TString of the length l to this TString.
    /** \param other: other String to append to this TString.
    \param length: How much characters of the other TString to add to this one. */
    TString<T, TAlloc>& append(const TString<T, TAlloc>& other, u32 length) {
        length = length > other.size() ? other.size() : length;
        if(0 == length) {
            return *this;
        }
        return append(other.mBuffer, length);
    }


    //! Reserves some memory.
    /** \param count: Amount of characters to reserve. */
    void reserve(u32 count) {
        if(count > mAllocated) {
            reallocate(count);
        }
    }


    //! finds first occurrence of character in TString
    /** \param c: Character to search for.
    \return Position where the character has been found,
    or -1 if not found. */
    s32 findFirst(T c) const {
        for(u32 i = 0; i < mUsed - 1; ++i)
            if(mBuffer[i] == c)
                return i;

        return -1;
    }

    //! finds first occurrence of a character of a list in TString
    /** \param c: List of characters to find. For example if the method
    should find the first occurrence of 'a' or 'b', this parameter should be "ab".
    \param count: Amount of characters in the list. Usually,
    this should be strlen(c)
    \return Position where one of the characters has been found,
    or -1 if not found. */
    s32 findFirstChar(const T* const c, u32 count = 1) const {
        if(!c || !count)
            return -1;

        for(u32 i = 0; i < mUsed - 1; ++i)
            for(u32 j = 0; j < count; ++j)
                if(mBuffer[i] == c[j])
                    return i;

        return -1;
    }


    //! Finds first position of a character not in a given list.
    /** \param c: List of characters not to find. For example if the method
    should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
    \param count: Amount of characters in the list. Usually,
    this should be strlen(c)
    \return Position where the character has been found,
    or -1 if not found. */
    template <class B>
    s32 findFirstCharNotInList(const B* const c, u32 count = 1) const {
        if(!c || !count)
            return -1;

        for(u32 i = 0; i < mUsed - 1; ++i) {
            u32 j;
            for(j = 0; j < count; ++j)
                if(mBuffer[i] == c[j])
                    break;

            if(j == count)
                return i;
        }

        return -1;
    }

    //! Finds last position of a character not in a given list.
    /** \param c: List of characters not to find. For example if the method
    should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
    \param count: Amount of characters in the list. Usually,
    this should be strlen(c)
    \return Position where the character has been found,
    or -1 if not found. */
    template <class B>
    s32 findLastCharNotInList(const B* const c, u32 count = 1) const {
        if(!c || !count)
            return -1;

        for(s32 i = (s32)(mUsed - 2); i >= 0; --i) {
            u32 j;
            for(j = 0; j < count; ++j)
                if(mBuffer[i] == c[j])
                    break;

            if(j == count)
                return i;
        }

        return -1;
    }

    //! finds next occurrence of character in TString
    /** \param c: Character to search for.
    \param startPos: Position in TString to start searching.
    \return Position where the character has been found,
    or -1 if not found. */
    s32 findNext(T c, u32 startPos) const {
        for(u32 i = startPos; i < mUsed - 1; ++i)
            if(mBuffer[i] == c)
                return i;

        return -1;
    }


    //! finds last occurrence of character in TString
    /** \param c: Character to search for.
    \param start: start to search reverse ( default = -1, on end )
    \return Position where the character has been found,
    or -1 if not found. */
    s32 findLast(T c, s32 start = -1) const {
        start = clamp(start < 0 ? (s32)(mUsed)-2 : start, 0, (s32)(mUsed)-2);
        for(s32 i = start; i >= 0; --i)
            if(mBuffer[i] == c)
                return i;

        return -1;
    }

    //! finds last occurrence of a character of a list in TString
    /** \param c: List of strings to find. For example if the method
    should find the last occurrence of 'a' or 'b', this parameter should be "ab".
    \param count: Amount of characters in the list. Usually,
    this should be strlen(c)
    \return Position where one of the characters has been found,
    or -1 if not found. */
    s32 findLastChar(const T* const c, u32 count = 1) const {
        if(!c || !count)
            return -1;

        for(s32 i = (s32)mUsed - 2; i >= 0; --i)
            for(u32 j = 0; j < count; ++j)
                if(mBuffer[i] == c[j])
                    return i;

        return -1;
    }


    //! finds another TString in this TString
    /** \param str: Another TString
    \param start: Start position of the search
    \return Positions where the TString has been found,
    or -1 if not found. */
    template <class B>
    s32 find(const B* const str, const u32 start = 0) const {
        if(str && *str) {
            u32 len = 0;

            while(str[len])
                ++len;

            if(len > mUsed - 1)
                return -1;

            for(u32 i = start; i < mUsed - len; ++i) {
                u32 j = 0;

                while(str[j] && mBuffer[i + j] == str[j])
                    ++j;

                if(!str[j])
                    return i;
            }
        }

        return -1;
    }


    //! Returns a substring
    /** \param begin Start of substring.
    \param length Length of substring.
    \param iLower copy only lower case */
    TString<T> subString(u32 begin, s32 length, bool iLower = false) const {
        // if start after TString or no proper substring length
        if((length <= 0) || (begin >= size()))
            return TString<T>("");

        // clamp length to maximal value
        if((length + begin) > size())
            length = size() - begin;

        TString<T> o;
        o.reserve(length + 1);

        s32 i;
        if(!iLower) {
            for(i = 0; i < length; ++i)
                o.mBuffer[i] = mBuffer[i + begin];
        } else {
            for(i = 0; i < length; ++i)
                o.mBuffer[i] = App2Lower(mBuffer[i + begin]);
        }

        o.mBuffer[length] = 0;
        o.mUsed = length + 1;

        return o;
    }


    //! Appends a character to this TString
    /** \param c Character to append. */
    TString<T, TAlloc>& operator+=(T c) {
        append(c);
        return *this;
    }


    //! Appends a char TString to this TString
    /** \param c Char TString to append. */
    TString<T, TAlloc>& operator+=(const T* const c) {
        append(c);
        return *this;
    }


    TString<T, TAlloc>& operator+=(const TString<T, TAlloc>& other) {
        return append(other);
    }

    TString<T, TAlloc>& operator=(const s32 val) {
        s8 tmpbuf[16];
        setUsed(0);
        append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%d", val));
        return *this;
    }

    TString<T, TAlloc>& operator=(const u32 val) {
        s8 tmpbuf[16];
        setUsed(0);
        append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%u", val));
        return *this;
    }

    TString<T, TAlloc>& operator+=(const s32 val) {
        s8 tmpbuf[16];
        append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%d", val));
        return *this;
    }


    TString<T, TAlloc>& operator+=(const u32 val) {
        s8 tmpbuf[16];
        append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%u", val));
        return *this;
    }

    TString<T, TAlloc>& operator=(const f64 val) {
        setUsed(0);
        s8 tmpbuf[255];
        snprintf(tmpbuf, sizeof(tmpbuf), "%0.6lf", val);
        append(tmpbuf);
        return *this;
    }

    TString<T, TAlloc>& operator+=(const f64 val) {
        s8 tmpbuf[255];
        snprintf(tmpbuf, sizeof(tmpbuf), "%0.6lf", val);
        append(tmpbuf);
        return *this;
    }

    TString<T, TAlloc>& operator=(const f32 val) {
        setUsed(0);
        s8 tmpbuf[255];
        snprintf(tmpbuf, sizeof(tmpbuf), "%0.6f", val);
        append(tmpbuf);
        return *this;
    }

    TString<T, TAlloc>& operator+=(const f32 val) {
        s8 tmpbuf[255];
        snprintf(tmpbuf, sizeof(tmpbuf), "%0.6f", val);
        append(tmpbuf);
        return *this;
    }


    //! Replaces all characters of a special type with another one
    /** \param toReplace Character to replace.
    \param replaceWith Character replacing the old one. */
    TString<T, TAlloc>& replace(T toReplace, T replaceWith) {
        for(u32 i = 0; i < mUsed - 1; ++i) {
            if(mBuffer[i] == toReplace)
                mBuffer[i] = replaceWith;
        }
        return *this;
    }


    //! Replaces all instances of a TString with another one.
    /** \param toReplace The TString to replace.
    \param replaceWith The TString replacing the old one. */
    TString<T, TAlloc>& replace(const TString<T, TAlloc>& toReplace, const TString<T, TAlloc>& replaceWith) {
        if(toReplace.size() == 0)
            return *this;

        const T* other = toReplace.c_str();
        const T* replace = replaceWith.c_str();
        const u32 other_size = toReplace.size();
        const u32 replace_size = replaceWith.size();

        // Determine the delta.  The algorithm will change depending on the delta.
        s32 delta = replace_size - other_size;

        // A character for character replace.  The TString will not shrink or grow.
        if(delta == 0) {
            s32 pos = 0;
            while((pos = find(other, pos)) != -1) {
                for(u32 i = 0; i < replace_size; ++i)
                    mBuffer[pos + i] = replace[i];
                ++pos;
            }
            return *this;
        }

        // We are going to be removing some characters.  The TString will shrink.
        if(delta < 0) {
            u32 i = 0;
            for(u32 pos = 0; pos < mUsed; ++i, ++pos) {
                // Is this potentially a match?
                if(mBuffer[pos] == *other) {
                    // Check to see if we have a match.
                    u32 j;
                    for(j = 0; j < other_size; ++j) {
                        if(mBuffer[pos + j] != other[j])
                            break;
                    }

                    // If we have a match, replace characters.
                    if(j == other_size) {
                        for(j = 0; j < replace_size; ++j)
                            mBuffer[i + j] = replace[j];
                        i += replace_size - 1;
                        pos += other_size - 1;
                        continue;
                    }
                }

                // No match found, just copy characters.
                mBuffer[i] = mBuffer[pos];
            }
            mBuffer[i - 1] = 0;
            mUsed = i;

            return *this;
        }

        // We are going to be adding characters, so the TString size will increase.
        // Count the number of times toReplace exists in the TString so we can allocate the new size.
        u32 find_count = 0;
        s32 pos = 0;
        while((pos = find(other, pos)) != -1) {
            ++find_count;
            ++pos;
        }

        // Re-allocate the TString now, if needed.
        u32 len = delta * find_count;
        if(mUsed + len > mAllocated)
            reallocate(mUsed + len);

        // Start replacing.
        pos = 0;
        while((pos = find(other, pos)) != -1) {
            T* start = mBuffer + pos + other_size - 1;
            T* ptr = mBuffer + mUsed - 1;
            T* end = mBuffer + delta + mUsed - 1;

            // Shift characters to make room for the TString.
            while(ptr != start) {
                *end = *ptr;
                --ptr;
                --end;
            }

            // Add the new TString now.
            for(u32 i = 0; i < replace_size; ++i)
                mBuffer[pos + i] = replace[i];

            pos += replace_size;
            mUsed += delta;
        }

        return *this;
    }


    //! Removes characters from a TString.
    /** \param c: Character to remove. */
    TString<T, TAlloc>& remove(T c) {
        u32 pos = 0;
        u32 found = 0;
        for(u32 i = 0; i < mUsed - 1; ++i) {
            if(mBuffer[i] == c) {
                ++found;
                continue;
            }

            mBuffer[pos++] = mBuffer[i];
        }
        mUsed -= found;
        mBuffer[mUsed - 1] = 0;
        return *this;
    }


    //! Removes a TString from the TString.
    /** \param toRemove: String to remove. */
    TString<T, TAlloc>& remove(const TString<T, TAlloc>& toRemove) {
        u32 size = toRemove.size();
        if(size == 0)
            return *this;
        u32 pos = 0;
        u32 found = 0;
        for(u32 i = 0; i < mUsed - 1; ++i) {
            u32 j = 0;
            while(j < size) {
                if(mBuffer[i + j] != toRemove[j])
                    break;
                ++j;
            }
            if(j == size) {
                found += size;
                i += size - 1;
                continue;
            }

            mBuffer[pos++] = mBuffer[i];
        }
        mUsed -= found;
        mBuffer[mUsed - 1] = 0;
        return *this;
    }


    //! Removes characters from a TString.
    /** \param characters: Characters to remove. */
    TString<T, TAlloc>& removeChars(const TString<T, TAlloc>& characters) {
        if(characters.size() == 0)
            return *this;

        u32 pos = 0;
        u32 found = 0;
        for(u32 i = 0; i < mUsed - 1; ++i) {
            // Don't use characters.findFirst as it finds the \0,
            // causing mUsed to become incorrect.
            bool docontinue = false;
            for(u32 j = 0; j < characters.size(); ++j) {
                if(characters[j] == mBuffer[i]) {
                    ++found;
                    docontinue = true;
                    break;
                }
            }
            if(docontinue)
                continue;

            mBuffer[pos++] = mBuffer[i];
        }
        mUsed -= found;
        mBuffer[mUsed - 1] = 0;

        return *this;
    }


    //! Trims the TString.
    /** Removes the specified characters (by default, Latin-1 whitespace)
    from the begining and the end of the TString. */
    TString<T, TAlloc>& trim(const TString<T, TAlloc>& whitespace = " \t\n\r") {
        // find start and end of the substring without the specified characters
        const s32 begin = findFirstCharNotInList(whitespace.c_str(), whitespace.mUsed);
        if(begin == -1)
            return (*this = "");

        const s32 end = findLastCharNotInList(whitespace.c_str(), whitespace.mUsed);

        return (*this = subString(begin, (end + 1) - begin));
    }


    //! Erases a character from the TString.
    /** May be slow, because all elements
    following after the erased element have to be copied.
    \param index: Index of element to be erased. */
    TString<T, TAlloc>& erase(u32 index) {
        APP_ASSERT(index < mUsed); // access violation

        for(u32 i = index + 1; i < mUsed; ++i) {
            mBuffer[i - 1] = mBuffer[i];
        }
        --mUsed;
        return *this;
    }

    //! verify the existing TString.
    TString<T, TAlloc>& validate() {
        // terminate on existing null
        for(u32 i = 0; i < mAllocated; ++i) {
            if(mBuffer[i] == 0) {
                mUsed = i + 1;
                return *this;
            }
        }

        // terminate
        if(mAllocated > 0) {
            mUsed = mAllocated;
            mBuffer[mUsed - 1] = 0;
        } else {
            mUsed = 0;
        }

        return *this;
    }

    //! gets the last char of a TString or null
    T lastChar() const {
        return mUsed > 1 ? mBuffer[mUsed - 2] : 0;
    }

    //! split TString into parts.
    /** This method will split a TString at certain delimiter characters
    into the container passed in as reference. The type of the container
    has to be given as template parameter. It must provide a pushBack and
    a size method.
    \param ret The result container
    \param c C-style TString of delimiter characters
    \param count Number of delimiter characters
    \param ignoreEmptyTokens Flag to avoid empty substrings in the result
    container. If two delimiters occur without a character in between, an
    empty substring would be placed in the result. If this flag is set,
    only non-empty strings are stored.
    \param keepSeparators Flag which allows to add the separator to the
    result TString. If this flag is true, the concatenation of the
    substrings results in the original TString. Otherwise, only the
    characters between the delimiters are returned.
    \return The number of resulting substrings
    */
    template<class container>
    u32 split(container& ret, const T* const c, u32 count = 1, bool ignoreEmptyTokens = true,
        bool keepSeparators = false) const {
        if(!c) {
            return 0;
        }
        const u32 oldSize = ret.size();
        u32 lastpos = 0;
        bool lastWasSeparator = false;
        for(u32 i = 0; i < mUsed; ++i) {
            bool foundSeparator = false;
            for(u32 j = 0; j < count; ++j) {
                if(mBuffer[i] == c[j]) {
                    if((!ignoreEmptyTokens || i - lastpos != 0) &&
                        !lastWasSeparator)
                        ret.pushBack(TString<T, TAlloc>(&mBuffer[lastpos], i - lastpos));
                    foundSeparator = true;
                    lastpos = (keepSeparators ? i : i + 1);
                    break;
                }
            }
            lastWasSeparator = foundSeparator;
        }

        if((mUsed - 1) > lastpos) {
            ret.pushBack(TString<T, TAlloc>(&mBuffer[lastpos], (mUsed - 1) - lastpos));
        }
        return ret.size() - oldSize;
    }

protected:

    //! Reallocate the mBuffer, make it bigger or smaller
    void reallocate(u32 new_size) {
        T* old_array = mBuffer;

        mBuffer = mAllocator.allocate(new_size); //new T[new_size];
        mAllocated = new_size;

        u32 amount = mUsed < new_size ? mUsed : new_size;
        for(u32 i = 0; i < amount; ++i) {
            mBuffer[i] = old_array[i];
        }
        if(mAllocated < mUsed) {
            mUsed = mAllocated;
        }
        mAllocator.deallocate(old_array); // delete [] old_array;
    }


    T* mBuffer;
    u32 mAllocated;
    u32 mUsed;
    TAlloc mAllocator;
};


//! Typedef for character strings
typedef TString<s8> CString;

//! Typedef for wide character strings
typedef TString<wchar_t> CWString;


//! used for all file system related strings.
/** This type will transparently handle different file system encodings. */
class CPath : public TString<tchar> {
public:
    CPath() : TString<tchar>() {
    }
    CPath(const TString<tchar>& val) : TString<tchar>(val) {
    }
    template <class B>
    CPath(const B* const c) : TString<tchar>(c) {
    }


    CPath& operator=(const TString<tchar>& other) {
        TString<tchar>::operator=(other);
        return *this;
    }

    CPath& operator=(const tchar* other) {
        TString<tchar>::operator=(other);
        return *this;
    }

    /**
    * @brief splits a path into components
    */
    void splitFilename(CPath* path = 0, CPath* filename = 0,
        CPath* extension = 0, bool make_lower = false)const {
        s32 i = size();
        s32 extpos = i;

        // search for path separator or beginning
        while(i >= 0) {
            if(mBuffer[i] == '.') {
                extpos = i;
                if(extension) {
                    *extension = subString(extpos + 1, size() - (extpos + 1), make_lower);
                }
            } else if(mBuffer[i] == '/' || mBuffer[i] == '\\') {
                if(filename)
                    *filename = subString(i + 1, extpos - (i + 1), make_lower);
                if(path) {
                    *path = subString(0, i + 1, make_lower);
                    path->replace('\\', '/');
                }
                return;
            }
            i -= 1;
        }
        if(filename) {
            *filename = subString(0, extpos, make_lower);
        }
    }


    //! delete path from filename
    CPath& deletePathFromFilename() {
        tchar* s = mBuffer;
        const tchar* p = s + size();
        // search for path separator or beginning
        while(*p != '/' && *p != '\\' && p != s) {
            p--;
        }
        if(p != s) {
            ++p;
            //*this = p;
            for(const tchar* epos = s + mUsed; p<epos; ++p){
                s[0] = p[0];
                ++s;
            }
        }
        return *this;
    }

    /**
    * @brief get the filename extension from a file path
    */
    CPath& getFileNameExtension(CPath& dest) const {
        s32 endPos = findLast('.');
        if(endPos < 0) {
            dest = "";
        } else {
            dest = subString(endPos, size());
        }
        return dest;
    }

    /**
    * @brief search if a filename has a proper extension
    */
    s32 isFileExtension(const CPath& ext0,
        const CPath& ext1, const CPath& ext2)const {
        s32 extPos = findLast('.');
        if(extPos < 0) {
            return 0;
        }
        extPos += 1;
        if(equals_substring_ignore_case(ext0, extPos)) {
            return 1;
        }
        if(equals_substring_ignore_case(ext1, extPos)) {
            return 2;
        }
        if(equals_substring_ignore_case(ext2, extPos)) {
            return 3;
        }
        return 0;
    }

    /**
    * @brief search if a filename has a proper extension
    */
    bool hasFileExtension(const CPath& ext0)const {
        s32 extPos = findLast('.');
        if(extPos < 0) {
            return 0;
        }
        extPos += 1;
        return equals_substring_ignore_case(ext0, extPos);
    }


    /**
    * @brief cut the filename extension from a source file path and store it in a dest file path
    */
    CPath& cutFilenameExtension(CPath& dest)const {
        s32 endPos = findLast('.');
        dest = subString(0, endPos < 0 ? size() : endPos);
        return dest;
    }


    //! looks if file is in the same directory of path. returns offset of directory.
    //! 0 means in same directory. 1 means file is direct child of path
    s32 isInSameDirectory(const CPath& file)const {
        s32 subA = 0;
        s32 subB = 0;
        if(size() && !equalsn(file, size())) {
            return -1;
        }
        s32 pos = 0;
        while((pos = findNext('/', pos)) >= 0) {
            subA += 1;
            pos += 1;
        }
        pos = 0;
        while((pos = file.findNext('/', pos)) >= 0) {
            subB += 1;
            pos += 1;
        }
        return subB - subA;
    }

    //! trim paths
    CPath& deletePathFromPath(s32 pathCount) {
        // delete path from filename
        s32 i = size();

        // search for path separator or beginning
        while(i >= 0) {
            if(mBuffer[i] == '/' || mBuffer[i] == '\\') {
                if(--pathCount <= 0) {
                    break;
                }
            }
            --i;
        }
        if(i > 0) {
            mBuffer[i + 1] = 0;
            validate();
        } else {
            *this = "";
        }
        return *this;
    }

};

} //namespace core
} //namespace app

#endif

