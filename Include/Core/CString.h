// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine" and the "irrXML" project.
// For conditions of distribution and use, see copyright notice in irrlicht.h and irrXML.h

#ifndef __IRR_STRING_H_INCLUDED__
#define __IRR_STRING_H_INCLUDED__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "HConfig.h"
#include "TAllocator.h"
#include "AppMath.h"


namespace app {
namespace core {

//! Returns a character converted to lower case
static inline u32 locale_lower(u32 x) {
    return x >= 'A' && x <= 'Z' ? x + 0x20 : x;
}

//! Returns a character converted to upper case
static inline u32 locale_upper(u32 x) {
    return x >= 'a' && x <= 'z' ? x + ('A' - 'a') : x;
}


/**
* @brief Very simple TString class with some useful features.
*/
template <typename T, typename TAlloc = TAllocator<T> >
class TString {
public:

    typedef T char_type;

    //! Default constructor
    TString()
        : array(0), allocated(1), used(1) {
        array = allocator.allocate(1); // new T[1];
        array[0] = 0;
    }


    //! Constructor
    TString(const TString<T, TAlloc>& other)
        : array(0), allocated(0), used(0) {
        *this = other;
    }

    //! Constructor from other TString types
    template <class B, class A>
    TString(const TString<B, A>& other)
        : array(0), allocated(0), used(0) {
        *this = other;
    }


    //! Constructs a TString from a float
    explicit TString(const double number)
        : array(0), allocated(0), used(0) {
        s8 tmpbuf[255];
        snprintf(tmpbuf, 255, "%0.6f", number);
        *this = tmpbuf;
    }


    //! Constructs a TString from an int
    explicit TString(int number)
        : array(0), allocated(0), used(0) {
        // store if negative and make positive

        bool negative = false;
        if (number < 0) {
            number *= -1;
            negative = true;
        }

        // temporary buffer for 16 numbers

        s8 tmpbuf[16] = { 0 };
        u32 idx = 15;

        // special case '0'

        if (!number) {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers

        while (number && idx) {
            --idx;
            tmpbuf[idx] = (s8)('0' + (number % 10));
            number /= 10;
        }

        // add sign

        if (negative) {
            --idx;
            tmpbuf[idx] = '-';
        }

        *this = &tmpbuf[idx];
    }


    //! Constructs a TString from an unsigned int
    explicit TString(unsigned int number)
        : array(0), allocated(0), used(0) {
        // temporary buffer for 16 numbers

        s8 tmpbuf[16] = { 0 };
        u32 idx = 15;

        // special case '0'

        if (!number) {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers

        while (number && idx) {
            --idx;
            tmpbuf[idx] = (s8)('0' + (number % 10));
            number /= 10;
        }

        *this = &tmpbuf[idx];
    }


    //! Constructs a TString from a long
    explicit TString(long number)
        : array(0), allocated(0), used(0) {
        // store if negative and make positive

        bool negative = false;
        if (number < 0) {
            number *= -1;
            negative = true;
        }

        // temporary buffer for 16 numbers

        s8 tmpbuf[16] = { 0 };
        u32 idx = 15;

        // special case '0'

        if (!number) {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers

        while (number && idx) {
            --idx;
            tmpbuf[idx] = (s8)('0' + (number % 10));
            number /= 10;
        }

        // add sign

        if (negative) {
            --idx;
            tmpbuf[idx] = '-';
        }

        *this = &tmpbuf[idx];
    }


    //! Constructs a TString from an unsigned long
    explicit TString(unsigned long number)
        : array(0), allocated(0), used(0) {
        // temporary buffer for 16 numbers

        s8 tmpbuf[16] = { 0 };
        u32 idx = 15;

        // special case '0'

        if (!number) {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers

        while (number && idx) {
            --idx;
            tmpbuf[idx] = (s8)('0' + (number % 10));
            number /= 10;
        }

        *this = &tmpbuf[idx];
    }


    //! Constructor for copying a TString from a pointer with a given length
    template <class B>
    TString(const B* const c, u32 length)
        : array(0), allocated(0), used(0) {
        if (!c) {
            // correctly init the TString to an empty one
            *this = "";
            return;
        }

        allocated = used = length + 1;
        array = allocator.allocate(used); // new T[used];

        for (u32 l = 0; l < length; ++l)
            array[l] = (T)c[l];

        array[length] = 0;
    }


    //! Constructor for unicode and ascii strings
    template <class B>
    TString(const B* const c)
        : array(0), allocated(0), used(0) {
        *this = c;
    }


    //! Destructor
    ~TString() {
        allocator.deallocate(array); // delete [] array;
    }


    //! Assignment operator
    TString<T, TAlloc>& operator=(const TString<T, TAlloc>& other) {
        if (this == &other)
            return *this;

        used = other.size() + 1;
        if (used > allocated) {
            allocator.deallocate(array); // delete [] array;
            allocated = used;
            array = allocator.allocate(used); //new T[used];
        }

        const T* p = other.c_str();
        for (u32 i = 0; i < used; ++i, ++p)
            array[i] = *p;

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
        if (!c) {
            if (!array) {
                array = allocator.allocate(1); //new T[1];
                allocated = 1;
            }
            used = 1;
            array[0] = 0x0;
            return *this;
        }

        if ((void*)c == (void*)array)
            return *this;

        u32 len = 0;
        const B* p = c;
        do {
            ++len;
        } while (*p++);

        // we'll keep the old TString for a while, because the new
        // TString could be a part of the current TString.
        T* oldArray = array;

        used = len;
        if (used > allocated) {
            allocated = used;
            array = allocator.allocate(used); //new T[used];
        }

        for (u32 l = 0; l < len; ++l)
            array[l] = (T)c[l];

        if (oldArray != array)
            allocator.deallocate(oldArray); // delete [] oldArray;

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
    T& operator [](const u32 index) {
        APP_ASSERT(index >= used) // bad index
            return array[index];
    }


    //! Direct access operator
    const T& operator [](const u32 index) const {
        APP_ASSERT(index >= used) // bad index
            return array[index];
    }


    //! Equality operator
    bool operator==(const T* const str) const {
        if (!str)
            return false;

        u32 i;
        for (i = 0; array[i] && str[i]; ++i)
            if (array[i] != str[i])
                return false;

        return (!array[i] && !str[i]);
    }


    //! Equality operator
    bool operator==(const TString<T, TAlloc>& other) const {
        for (u32 i = 0; array[i] && other.array[i]; ++i)
            if (array[i] != other.array[i])
                return false;

        return used == other.used;
    }


    //! Is smaller comparator
    bool operator<(const TString<T, TAlloc>& other) const {
        for (u32 i = 0; array[i] && other.array[i]; ++i) {
            const s32 diff = array[i] - other.array[i];
            if (diff)
                return (diff < 0);
        }

        return (used < other.used);
    }


    //! Inequality operator
    bool operator!=(const T* const str) const {
        return !(*this == str);
    }


    //! Inequality operator
    bool operator!=(const TString<T, TAlloc>& other) const {
        return !(*this == other);
    }


    //! Returns length of the TString's content
    /** \return Length of the TString's content in characters, excluding
    the trailing NUL. */
    u32 size() const {
        return used - 1;
    }

    //! Informs if the TString is empty or not.
    //! \return True if the TString is empty, false if not.
    bool empty() const {
        return (size() == 0);
    }

    //! Returns character TString
    /** \return pointer to C-style NUL terminated TString. */
    const T* c_str() const {
        return array;
    }


    //! Makes the TString lower case.
    TString<T, TAlloc>& make_lower() {
        for (u32 i = 0; array[i]; ++i)
            array[i] = locale_lower(array[i]);
        return *this;
    }


    //! Makes the TString upper case.
    TString<T, TAlloc>& make_upper() {
        for (u32 i = 0; array[i]; ++i)
            array[i] = locale_upper(array[i]);
        return *this;
    }


    //! Compares the strings ignoring case.
    /** \param other: Other TString to compare.
    \return True if the strings are equal ignoring case. */
    bool equals_ignore_case(const TString<T, TAlloc>& other) const {
        for (u32 i = 0; array[i] && other[i]; ++i)
            if (locale_lower(array[i]) != locale_lower(other[i]))
                return false;

        return used == other.used;
    }

    //! Compares the strings ignoring case.
    /** \param other: Other TString to compare.
        \param sourcePos: where to start to compare in the TString
    \return True if the strings are equal ignoring case. */
    bool equals_substring_ignore_case(const TString<T, TAlloc>&other, const s32 sourcePos = 0) const {
        if ((u32)sourcePos >= used)
            return false;

        u32 i;
        for (i = 0; array[sourcePos + i] && other[i]; ++i)
            if (locale_lower(array[sourcePos + i]) != locale_lower(other[i]))
                return false;

        return array[sourcePos + i] == 0 && other[i] == 0;
    }


    //! Compares the strings ignoring case.
    /** \param other: Other TString to compare.
    \return True if this TString is smaller ignoring case. */
    bool lower_ignore_case(const TString<T, TAlloc>& other) const {
        for (u32 i = 0; array[i] && other.array[i]; ++i) {
            s32 diff = (s32)locale_lower(array[i]) - (s32)locale_lower(other.array[i]);
            if (diff)
                return diff < 0;
        }

        return used < other.used;
    }


    //! compares the first n characters of the strings
    /** \param other Other TString to compare.
    \param n Number of characters to compare
    \return True if the n first characters of both strings are equal. */
    bool equalsn(const TString<T, TAlloc>& other, u32 n) const {
        u32 i;
        for (i = 0; array[i] && other[i] && i < n; ++i)
            if (array[i] != other[i])
                return false;

        // if one (or both) of the strings was smaller then they
        // are only equal if they have the same length
        return (i == n) || (used == other.used);
    }


    //! compares the first n characters of the strings
    /** \param str Other TString to compare.
    \param n Number of characters to compare
    \return True if the n first characters of both strings are equal. */
    bool equalsn(const T* const str, u32 n) const {
        if (!str)
            return false;
        u32 i;
        for (i = 0; array[i] && str[i] && i < n; ++i)
            if (array[i] != str[i])
                return false;

        // if one (or both) of the strings was smaller then they
        // are only equal if they have the same length
        return (i == n) || (array[i] == 0 && str[i] == 0);
    }


    //! Appends a character to this TString
    /** \param character: Character to append. */
    TString<T, TAlloc>& append(T character) {
        if (used + 1 > allocated)
            reallocate(used + 1);

        ++used;

        array[used - 2] = character;
        array[used - 1] = 0;

        return *this;
    }


    //! Appends a char TString to this TString
    /** \param other: Char TString to append. */
    /** \param length: The length of the TString to append. */
    TString<T, TAlloc>& append(const T* const other, u32 length = 0xffffffff) {
        if (!other)
            return *this;

        u32 len = 0;
        const T* p = other;
        while (*p) {
            ++len;
            ++p;
        }
        if (len > length)
            len = length;

        if (used + len > allocated)
            reallocate(used + len);

        --used;
        ++len;

        for (u32 l = 0; l < len; ++l)
            array[l + used] = *(other + l);

        used += len;

        return *this;
    }


    //! Appends a TString to this TString
    /** \param other: String to append. */
    TString<T, TAlloc>& append(const TString<T, TAlloc>& other) {
        if (other.size() == 0)
            return *this;

        --used;
        u32 len = other.size() + 1;

        if (used + len > allocated)
            reallocate(used + len);

        for (u32 l = 0; l < len; ++l)
            array[used + l] = other[l];

        used += len;

        return *this;
    }


    //! Appends a TString of the length l to this TString.
    /** \param other: other String to append to this TString.
    \param length: How much characters of the other TString to add to this one. */
    TString<T, TAlloc>& append(const TString<T, TAlloc>& other, u32 length) {
        if (other.size() == 0)
            return *this;

        if (other.size() < length) {
            append(other);
            return *this;
        }

        if (used + length > allocated)
            reallocate(used + length);

        --used;

        for (u32 l = 0; l < length; ++l)
            array[l + used] = other[l];
        used += length;

        // ensure proper termination
        array[used] = 0;
        ++used;

        return *this;
    }


    //! Reserves some memory.
    /** \param count: Amount of characters to reserve. */
    void reserve(u32 count) {
        if (count < allocated)
            return;

        reallocate(count);
    }


    //! finds first occurrence of character in TString
    /** \param c: Character to search for.
    \return Position where the character has been found,
    or -1 if not found. */
    s32 findFirst(T c) const {
        for (u32 i = 0; i < used - 1; ++i)
            if (array[i] == c)
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
        if (!c || !count)
            return -1;

        for (u32 i = 0; i < used - 1; ++i)
            for (u32 j = 0; j < count; ++j)
                if (array[i] == c[j])
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
        if (!c || !count)
            return -1;

        for (u32 i = 0; i < used - 1; ++i) {
            u32 j;
            for (j = 0; j < count; ++j)
                if (array[i] == c[j])
                    break;

            if (j == count)
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
        if (!c || !count)
            return -1;

        for (s32 i = (s32)(used - 2); i >= 0; --i) {
            u32 j;
            for (j = 0; j < count; ++j)
                if (array[i] == c[j])
                    break;

            if (j == count)
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
        for (u32 i = startPos; i < used - 1; ++i)
            if (array[i] == c)
                return i;

        return -1;
    }


    //! finds last occurrence of character in TString
    /** \param c: Character to search for.
    \param start: start to search reverse ( default = -1, on end )
    \return Position where the character has been found,
    or -1 if not found. */
    s32 findLast(T c, s32 start = -1) const {
        start = core::clamp(start < 0 ? (s32)(used)-2 : start, 0, (s32)(used)-2);
        for (s32 i = start; i >= 0; --i)
            if (array[i] == c)
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
        if (!c || !count)
            return -1;

        for (s32 i = (s32)used - 2; i >= 0; --i)
            for (u32 j = 0; j < count; ++j)
                if (array[i] == c[j])
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
        if (str && *str) {
            u32 len = 0;

            while (str[len])
                ++len;

            if (len > used - 1)
                return -1;

            for (u32 i = start; i < used - len; ++i) {
                u32 j = 0;

                while (str[j] && array[i + j] == str[j])
                    ++j;

                if (!str[j])
                    return i;
            }
        }

        return -1;
    }


    //! Returns a substring
    /** \param begin Start of substring.
    \param length Length of substring.
    \param make_lower copy only lower case */
    TString<T> subString(u32 begin, s32 length, bool make_lower = false) const {
        // if start after TString
        // or no proper substring length
        if ((length <= 0) || (begin >= size()))
            return TString<T>("");
        // clamp length to maximal value
        if ((length + begin) > size())
            length = size() - begin;

        TString<T> o;
        o.reserve(length + 1);

        s32 i;
        if (!make_lower) {
            for (i = 0; i < length; ++i)
                o.array[i] = array[i + begin];
        } else {
            for (i = 0; i < length; ++i)
                o.array[i] = locale_lower(array[i + begin]);
        }

        o.array[length] = 0;
        o.used = length + 1;

        return o;
    }


    //! Appends a character to this TString
    /** \param c Character to append. */
    TString<T, TAlloc>& operator += (T c) {
        append(c);
        return *this;
    }


    //! Appends a char TString to this TString
    /** \param c Char TString to append. */
    TString<T, TAlloc>& operator += (const T* const c) {
        append(c);
        return *this;
    }


    //! Appends a TString to this TString
    /** \param other String to append. */
    TString<T, TAlloc>& operator += (const TString<T, TAlloc>& other) {
        append(other);
        return *this;
    }


    //! Appends a TString representation of a number to this TString
    /** \param i Number to append. */
    TString<T, TAlloc>& operator += (const int i) {
        append(TString<T, TAlloc>(i));
        return *this;
    }


    //! Appends a TString representation of a number to this TString
    /** \param i Number to append. */
    TString<T, TAlloc>& operator += (const unsigned int i) {
        append(TString<T, TAlloc>(i));
        return *this;
    }


    //! Appends a TString representation of a number to this TString
    /** \param i Number to append. */
    TString<T, TAlloc>& operator += (const long i) {
        append(TString<T, TAlloc>(i));
        return *this;
    }


    //! Appends a TString representation of a number to this TString
    /** \param i Number to append. */
    TString<T, TAlloc>& operator += (const unsigned long i) {
        append(TString<T, TAlloc>(i));
        return *this;
    }


    //! Appends a TString representation of a number to this TString
    /** \param i Number to append. */
    TString<T, TAlloc>& operator += (const double i) {
        append(TString<T, TAlloc>(i));
        return *this;
    }


    //! Appends a TString representation of a number to this TString
    /** \param i Number to append. */
    TString<T, TAlloc>& operator += (const float i) {
        append(TString<T, TAlloc>(i));
        return *this;
    }


    //! Replaces all characters of a special type with another one
    /** \param toReplace Character to replace.
    \param replaceWith Character replacing the old one. */
    TString<T, TAlloc>& replace(T toReplace, T replaceWith) {
        for (u32 i = 0; i < used - 1; ++i)
            if (array[i] == toReplace)
                array[i] = replaceWith;
        return *this;
    }


    //! Replaces all instances of a TString with another one.
    /** \param toReplace The TString to replace.
    \param replaceWith The TString replacing the old one. */
    TString<T, TAlloc>& replace(const TString<T, TAlloc>& toReplace, const TString<T, TAlloc>& replaceWith) {
        if (toReplace.size() == 0)
            return *this;

        const T* other = toReplace.c_str();
        const T* replace = replaceWith.c_str();
        const u32 other_size = toReplace.size();
        const u32 replace_size = replaceWith.size();

        // Determine the delta.  The algorithm will change depending on the delta.
        s32 delta = replace_size - other_size;

        // A character for character replace.  The TString will not shrink or grow.
        if (delta == 0) {
            s32 pos = 0;
            while ((pos = find(other, pos)) != -1) {
                for (u32 i = 0; i < replace_size; ++i)
                    array[pos + i] = replace[i];
                ++pos;
            }
            return *this;
        }

        // We are going to be removing some characters.  The TString will shrink.
        if (delta < 0) {
            u32 i = 0;
            for (u32 pos = 0; pos < used; ++i, ++pos) {
                // Is this potentially a match?
                if (array[pos] == *other) {
                    // Check to see if we have a match.
                    u32 j;
                    for (j = 0; j < other_size; ++j) {
                        if (array[pos + j] != other[j])
                            break;
                    }

                    // If we have a match, replace characters.
                    if (j == other_size) {
                        for (j = 0; j < replace_size; ++j)
                            array[i + j] = replace[j];
                        i += replace_size - 1;
                        pos += other_size - 1;
                        continue;
                    }
                }

                // No match found, just copy characters.
                array[i] = array[pos];
            }
            array[i - 1] = 0;
            used = i;

            return *this;
        }

        // We are going to be adding characters, so the TString size will increase.
        // Count the number of times toReplace exists in the TString so we can allocate the new size.
        u32 find_count = 0;
        s32 pos = 0;
        while ((pos = find(other, pos)) != -1) {
            ++find_count;
            ++pos;
        }

        // Re-allocate the TString now, if needed.
        u32 len = delta * find_count;
        if (used + len > allocated)
            reallocate(used + len);

        // Start replacing.
        pos = 0;
        while ((pos = find(other, pos)) != -1) {
            T* start = array + pos + other_size - 1;
            T* ptr = array + used - 1;
            T* end = array + delta + used - 1;

            // Shift characters to make room for the TString.
            while (ptr != start) {
                *end = *ptr;
                --ptr;
                --end;
            }

            // Add the new TString now.
            for (u32 i = 0; i < replace_size; ++i)
                array[pos + i] = replace[i];

            pos += replace_size;
            used += delta;
        }

        return *this;
    }


    //! Removes characters from a TString.
    /** \param c: Character to remove. */
    TString<T, TAlloc>& remove(T c) {
        u32 pos = 0;
        u32 found = 0;
        for (u32 i = 0; i < used - 1; ++i) {
            if (array[i] == c) {
                ++found;
                continue;
            }

            array[pos++] = array[i];
        }
        used -= found;
        array[used - 1] = 0;
        return *this;
    }


    //! Removes a TString from the TString.
    /** \param toRemove: String to remove. */
    TString<T, TAlloc>& remove(const TString<T, TAlloc>& toRemove) {
        u32 size = toRemove.size();
        if (size == 0)
            return *this;
        u32 pos = 0;
        u32 found = 0;
        for (u32 i = 0; i < used - 1; ++i) {
            u32 j = 0;
            while (j < size) {
                if (array[i + j] != toRemove[j])
                    break;
                ++j;
            }
            if (j == size) {
                found += size;
                i += size - 1;
                continue;
            }

            array[pos++] = array[i];
        }
        used -= found;
        array[used - 1] = 0;
        return *this;
    }


    //! Removes characters from a TString.
    /** \param characters: Characters to remove. */
    TString<T, TAlloc>& removeChars(const TString<T, TAlloc> & characters) {
        if (characters.size() == 0)
            return *this;

        u32 pos = 0;
        u32 found = 0;
        for (u32 i = 0; i < used - 1; ++i) {
            // Don't use characters.findFirst as it finds the \0,
            // causing used to become incorrect.
            bool docontinue = false;
            for (u32 j = 0; j < characters.size(); ++j) {
                if (characters[j] == array[i]) {
                    ++found;
                    docontinue = true;
                    break;
                }
            }
            if (docontinue)
                continue;

            array[pos++] = array[i];
        }
        used -= found;
        array[used - 1] = 0;

        return *this;
    }


    //! Trims the TString.
    /** Removes the specified characters (by default, Latin-1 whitespace)
    from the begining and the end of the TString. */
    TString<T, TAlloc>& trim(const TString<T, TAlloc> & whitespace = " \t\n\r") {
        // find start and end of the substring without the specified characters
        const s32 begin = findFirstCharNotInList(whitespace.c_str(), whitespace.used);
        if (begin == -1)
            return (*this = "");

        const s32 end = findLastCharNotInList(whitespace.c_str(), whitespace.used);

        return (*this = subString(begin, (end + 1) - begin));
    }


    //! Erases a character from the TString.
    /** May be slow, because all elements
    following after the erased element have to be copied.
    \param index: Index of element to be erased. */
    TString<T, TAlloc>& erase(u32 index) {
        APP_ASSERT(index >= used) // access violation

            for (u32 i = index + 1; i < used; ++i)
                array[i - 1] = array[i];

        --used;
        return *this;
    }

    //! verify the existing TString.
    TString<T, TAlloc>& validate() {
        // terminate on existing null
        for (u32 i = 0; i < allocated; ++i) {
            if (array[i] == 0) {
                used = i + 1;
                return *this;
            }
        }

        // terminate
        if (allocated > 0) {
            used = allocated;
            array[used - 1] = 0;
        } else {
            used = 0;
        }

        return *this;
    }

    //! gets the last char of a TString or null
    T lastChar() const {
        return used > 1 ? array[used - 2] : 0;
    }

    //! split TString into parts.
    /** This method will split a TString at certain delimiter characters
    into the container passed in as reference. The type of the container
    has to be given as template parameter. It must provide a push_back and
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
    u32 split(container& ret, const T* const c, u32 count = 1, bool ignoreEmptyTokens = true, bool keepSeparators = false) const {
        if (!c)
            return 0;

        const u32 oldSize = ret.size();
        u32 lastpos = 0;
        bool lastWasSeparator = false;
        for (u32 i = 0; i < used; ++i) {
            bool foundSeparator = false;
            for (u32 j = 0; j < count; ++j) {
                if (array[i] == c[j]) {
                    if ((!ignoreEmptyTokens || i - lastpos != 0) &&
                        !lastWasSeparator)
                        ret.push_back(TString<T, TAlloc>(&array[lastpos], i - lastpos));
                    foundSeparator = true;
                    lastpos = (keepSeparators ? i : i + 1);
                    break;
                }
            }
            lastWasSeparator = foundSeparator;
        }
        if ((used - 1) > lastpos)
            ret.push_back(TString<T, TAlloc>(&array[lastpos], (used - 1) - lastpos));
        return ret.size() - oldSize;
    }

private:

    //! Reallocate the array, make it bigger or smaller
    void reallocate(u32 new_size) {
        T* old_array = array;

        array = allocator.allocate(new_size); //new T[new_size];
        allocated = new_size;

        u32 amount = used < new_size ? used : new_size;
        for (u32 i = 0; i < amount; ++i)
            array[i] = old_array[i];

        if (allocated < used)
            used = allocated;

        allocator.deallocate(old_array); // delete [] old_array;
    }

    //--- member variables
    T* array;
    u32 allocated;
    u32 used;
    TAlloc allocator;
};


//! Typedef for character strings
typedef TString<s8> CString;

//! Typedef for wide character strings
typedef TString<wchar_t> CWString;

//! Type used for all file system related strings.
/** This type will transparently handle different file system encodings. */
typedef TString<fschar_t> CPath;

} // end namespace core
} // end namespace app

#endif

