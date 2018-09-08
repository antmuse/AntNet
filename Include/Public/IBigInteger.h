#ifndef APP_IBIGINTEGER_H
#define APP_IBIGINTEGER_H

#include "HConfig.h"
#include "irrTypes.h"

namespace irr {

/// template for big integer
template<typename T>
class IBigInteger {
public:
    IBigInteger() {
        APP_ASSERT(sizeof(T) > sizeof(u64));
        //APP_ASSERT(0 == sizeof(T) % 2);
        *this = 0;
    }

    IBigInteger(u64 val) {
        APP_ASSERT(sizeof(T) > sizeof(u64));
        //APP_ASSERT(0 == sizeof(T) % 2);
        *this = val;
    }

    IBigInteger(const IBigInteger& other) {
        *this = other;
    }

    IBigInteger& operator=(const T& it) {
        u8* loc = ((u8*) &mData);
        u8* oter = (u8*) &it;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*loc++) = (*oter++);
        }
        return *this;
    }

    IBigInteger& operator=(const IBigInteger& other) {
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            *loc++ = *oter++;
        }
        return *this;
    }

    IBigInteger& operator=(const u64 other) {
        u32 diff = sizeof(T) - sizeof(u64);
        u8* loc = ((u8*) &mData);
        u8* oter = (u8*) &other;
#if defined(APP_ENDIAN_BIG)
        for(u32 i = 0; i < diff; ++i) {
            *loc++ = 0;
        }
        for(u32 i = sizeof(u64); i > 0; --i) {
            *loc++ = *oter++;
        }
#else
        for(u32 i = sizeof(u64); i > 0; --i) {
            *loc++ = *oter++;
        }
        for(u32 i = 0; i < diff; ++i) {
            *loc++ = 0;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    IBigInteger& operator--() {
#if defined(APP_ENDIAN_BIG)
        u8* loc = ((u8*) &mData) + sizeof(T) - 1;
        for(u32 i = 0; i < sizeof(T); ++i) {
            if(*loc > 0) {
                --(*loc);
                break;
            }
            *loc-- = 0xFF;
        }
#else
        u8* loc = (u8*) &mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            if(*loc > 0) {
                --(*loc);
                break;
            }
            *loc++ = 0xFF;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    IBigInteger& operator++() {
        u16 carry = 1;
        u16 tmp;
#if defined(APP_ENDIAN_BIG)
        u8* loc = ((u8*) &mData) + sizeof(T) - 1;
        for(u32 i = 0; i < sizeof(T); ++i) {
            tmp = (*loc) + carry;
            carry = (tmp & 0xff00) >> 8;
            *loc-- = tmp & 0x00ff;
            if(0 == carry) break;
        }
#else
        u8* loc = (u8*) &mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            tmp = (*loc) + carry;
            carry = (tmp & 0xff00) >> 8;
            *loc++ = tmp & 0x00ff;
            if(0 == carry) break;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    /**
    *@brief maybe slow, please use ++IBigInteger instead of IBigInteger++ as far as possible;
    */
    IBigInteger operator++(s32 it) {
        IBigInteger ret(*this);
        ++(*this);
        return ret;
    }

    /**
    *@brief maybe slow, please use --IBigInteger instead of IBigInteger-- as far as possible;
    */
    IBigInteger operator--(s32 it) {
        IBigInteger ret(*this);
        --(*this);
        return ret;
    }

    //TODO>>test
    IBigInteger operator*(const u64 other) {
        IBigInteger result;
#if defined(APP_ENDIAN_BIG)
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other;
        u32 mul;
        u32 carry;
        for(s32 i = sizeof(T) - 1; i >= 0; --i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(s32 j = sizeof(u64) - 1; j >= 0; --j) {
                    if((i + j) < 0) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff00) >> 8;
                    res[i + j] += (carry & 0xff00) >> 8;
                    carry += (mul & 0xff);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#else
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other;
        u32 mul;
        u32 carry;
        for(u32 i = 0; i < sizeof(T); ++i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(u32 j = 0; j < sizeof(u64); ++j) {
                    if((i + j) >= sizeof(T)) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff);
                    res[i + j] += u8(carry & 0xff);
                    carry += (mul & 0xff00);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#endif
        return result;
    }

    //TODO>>test
    IBigInteger operator*(const IBigInteger& other) {
        IBigInteger result;
#if defined(APP_ENDIAN_BIG)
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        u32 mul;
        u32 carry;
        for(s32 i = sizeof(T) - 1; i >= 0; --i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(s32 j = sizeof(T) - 1; j >= 0; --j) {
                    if((i + j) < 0) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff00) >> 8;
                    res[i + j] += (carry & 0xff00) >> 8;
                    carry += (mul & 0xff);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#else
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        u32 mul;
        u32 carry;
        for(u32 i = 0; i < sizeof(T); ++i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(u32 j = 0; j < sizeof(T); ++j) {
                    if((i + j) >= sizeof(T)) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff);
                    res[i + j] += u8(carry & 0xff);
                    carry += (mul & 0xff00);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#endif
        return result;
    }

    //TODO>>test
    IBigInteger& operator*=(const u64 other) {
        IBigInteger result;
#if defined(APP_ENDIAN_BIG)
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other;
        u32 mul;
        u32 carry;
        for(s32 i = sizeof(T) - 1; i >= 0; --i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(s32 j = sizeof(u64) - 1; j >= 0; --j) {
                    if((i + j) < 0) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff00) >> 8;
                    res[i + j] += (carry & 0xff00) >> 8;
                    carry += (mul & 0xff);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#else
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other;
        u32 mul;
        u32 carry;
        for(u32 i = 0; i < sizeof(T); ++i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(u32 j = 0; j < sizeof(u64); ++j) {
                    if((i + j) >= sizeof(T)) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff);
                    res[i + j] += u8(carry & 0xff);
                    carry += (mul & 0xff00);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#endif
        *this = result;
        return *this;
    }

    //TODO>>test
    IBigInteger& operator*=(const IBigInteger& other) {
        IBigInteger result;
#if defined(APP_ENDIAN_BIG)
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        u32 mul;
        u32 carry;
        for(s32 i = sizeof(T) - 1; i >= 0; --i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(s32 j = sizeof(T) - 1; j >= 0; --j) {
                    if((i + j) < 0) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff00) >> 8;
                    res[i + j] += (carry & 0xff00) >> 8;
                    carry += (mul & 0xff);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#else
        u8* res = (u8*) &result.mData;
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        u32 mul;
        u32 carry;
        for(u32 i = 0; i < sizeof(T); ++i) {
            mul = 0;
            carry = 0;
            if(loc[i] > 0) {
                for(u32 j = 0; j < sizeof(T); ++j) {
                    if((i + j) >= sizeof(T)) {
                        break;      //overflow
                    }
                    mul = (u32(loc[i])) * (u32(oter[j]));
                    carry += (mul & 0xff);
                    res[i + j] += u8(carry & 0xff);
                    carry += (mul & 0xff00);
                    carry >>= 8;    //1 byte
                }
            }
        }//for
#endif
        *this = result;
        return *this;
    }

    //TODO>>
    IBigInteger operator/(const u64 other) {
        return *this;
    }

    //TODO>>
    IBigInteger& operator/(const IBigInteger& other) {
        return *this;
    }

    //TODO>>
    IBigInteger& operator/=(const u64 other) {
        return *this;
    }

    //TODO>>
    IBigInteger& operator/=(const IBigInteger& other) {
        return *this;
    }

    IBigInteger& operator-=(u64 other) {
#if defined(APP_ENDIAN_BIG)
        u8* loc = ((u8*) &mData) + sizeof(T) - sizeof(u64);
        if((*(u64*) loc) >= other) {
            (*(u64*) loc) -= other;
            return *this;
        }
        other -= (*(u64*) loc);
        (*(u64*) loc) = ((u64) 0) - other;
        for(u32 i = sizeof(u64); i < sizeof(T); ++i) {
            if(*loc > 0) {
                --(*loc);
                break;
            }
            *loc-- = 0xFF;
        }
#else
        u8* loc = (u8*) &mData;
        if((*(u64*) loc) >= other) {
            (*(u64*) loc) -= other;
            return *this;
        }
        other -= (*(u64*) loc);
        (*(u64*) loc) = ((u64) 0) - other;
        loc += sizeof(u64);
        for(u32 i = sizeof(u64); i < sizeof(T); ++i) {
            if(*loc > 0) {
                --(*loc);
                break;
            }
            *loc++ = 0xFF;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    IBigInteger& operator-=(const IBigInteger& other) {
#if defined(APP_ENDIAN_BIG)
        u8* loc = ((u8*) &mData) + sizeof(T) - sizeof(u64);
#else
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        u8 loan = 0;
        for(u32 i = 0; i < sizeof(T); ++i) {
            if(0 == loan && (*loc) >= (*oter)) {
                (*loc++) -= (*oter++);
                continue;
            }
            if(1 == loan && (*loc) > (*oter)) {
                (*loc++) -= 1 + (*oter++);
                loan = 0;
                continue;
            }
            (*loc++) = ((u8) 0) - ((*oter++) - (*loc)) - loan;
            loan = 1;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    IBigInteger& operator+=(const u64 other) {
        u16 carry = 0;
        u16 tmp;
#if defined(APP_ENDIAN_BIG)
        u8* loc = ((u8*) &mData) + sizeof(T) - 1;
        const u8* oter = ((u8*) &other) + sizeof(u64) - 1;
        for(u32 i = 0; i < sizeof(u64); ++i) {
            tmp = (*loc) + (*oter--) + carry;
            carry = (tmp & 0xff00) >> 8;
            *loc-- = tmp & 0x00ff;
        }
#else
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other;
        for(u32 i = 0; i < sizeof(u64); ++i) {
            tmp = (*loc) + (*oter++) + carry;
            carry = (tmp & 0xff00) >> 8;
            *loc++ = tmp & 0x00ff;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    IBigInteger& operator+=(const IBigInteger& other) {
        u16 carry = 0;
        u16 tmp;
#if defined(APP_ENDIAN_BIG)
        u8* loc = ((u8*) &mData) + sizeof(T) - 1;
        const u8* oter = ((u8*) &other.mData) + sizeof(T) - 1;
        for(u32 i = 0; i < sizeof(T); ++i) {
            tmp = (*loc) + (*oter--) + carry;
            carry = (tmp & 0xff00) >> 8;
            *loc-- = tmp & 0x00ff;
        }
#else
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            tmp = (*loc) + (*oter++) + carry;
            carry = (tmp & 0xff00) >> 8;
            *loc++ = tmp & 0x00ff;
        }
#endif //APP_ENDIAN_BIG
        return *this;
    }

    bool operator==(const IBigInteger& other) const {
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc++ != *oter++) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const IBigInteger& other) const {
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc++ != *oter++) {
                return true;
            }
        }
        return false;
    }

    bool operator>(const IBigInteger& other) const {
#if defined(APP_ENDIAN_BIG)
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc++ > *oter++) {
                return true;
            }
        }
#else
        u8* loc = (u8*) &mData + sizeof(T) - 1;
        u8* oter = (u8*) &other.mData + sizeof(T) - 1;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc-- > *oter--) {
                return true;
            }
        }
#endif //APP_ENDIAN_BIG
        return false;
    }

    bool operator>=(const IBigInteger& other) const {
#if defined(APP_ENDIAN_BIG)
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc++ < *oter++) {
                return false;
            }
        }
#else
        u8* loc = (u8*) &mData + sizeof(T) - 1;
        u8* oter = (u8*) &other.mData + sizeof(T) - 1;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc-- < *oter--) {
                return false;
            }
        }
#endif //APP_ENDIAN_BIG
        return true;
    }

    bool operator<(const IBigInteger& other) const {
#if defined(APP_ENDIAN_BIG)
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc++ < *oter++) {
                return true;
            }
        }
#else
        u8* loc = (u8*) &mData + sizeof(T) - 1;
        u8* oter = (u8*) &other.mData + sizeof(T) - 1;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc-- < *oter--) {
                return true;
            }
        }
#endif //APP_ENDIAN_BIG
        return false;
    }

    bool operator<=(const IBigInteger& other) const {
#if defined(APP_ENDIAN_BIG)
        u8* loc = (u8*) &mData;
        u8* oter = (u8*) &other.mData;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc++ > *oter++) {
                return false;
            }
        }
#else
        u8* loc = (u8*) &mData + sizeof(T) - 1;
        u8* oter = (u8*) &other.mData + sizeof(T) - 1;
        for(u32 i = sizeof(T); i > 0; --i) {
            if(*loc-- > *oter--) {
                return false;
            }
        }
#endif //APP_ENDIAN_BIG
        return true;
    }

    IBigInteger& operator~() {
        u8* loc = (u8*) &mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*loc++) = ~(*loc);
        }
        return *this;
    }

    //TODO>>
    IBigInteger& operator>>(u32 it) {
        if(it > 0) {
            if(it >= sizeof(T)) {
                *this = 0;
                return *this;
            }
#if defined(APP_ENDIAN_BIG)

#else
            u32 bytes = it >> 3; // =it/8
            u32 bits = it - bytes;
            u8* loc = (u8*) &mData;
            u8* pos = loc + bytes;
            const u8* const end = loc + sizeof(T) - 2 * sizeof(u64);
            u64 step1 = 0;
            u64 step2 = 0;
            const u64 mask1 = 0xffffffffffffffff >> (sizeof(u64) - bits);
            const u64 mask2 = 0xffffffffffffffff << bits;
            for(; pos < end;) {
                step2 = *(u64*) (pos + sizeof(u64) - 1);
                step1 |= (*(u64*) pos) >> bits;
                step2 >>= bits;
                (*(u64*) loc) = step1 | (step2&mask1);
                pos += sizeof(u64);
                loc += sizeof(u64);
                step1 = step2 >> (8 - bits);
            }
#endif
        }
        return *this;
    }

    //TODO>>
    IBigInteger& operator<<(u32 it) {
        u8* loc = (u8*) &mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*loc++) = ~(*loc);
        }
        return *this;
    }

    IBigInteger& operator&=(const IBigInteger& other) {
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*loc++) &= (*oter++);
        }
        return *this;
    }

    IBigInteger& operator|=(const IBigInteger& other) {
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*loc++) |= (*oter++);
        }
        return *this;
    }

    IBigInteger& operator^=(const IBigInteger& other) {
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*loc++) ^= (*oter++);
        }
        return *this;
    }

    IBigInteger operator&(const IBigInteger& other) {
        IBigInteger ret;
        u8* rp = (u8*) &ret.mData;
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*rp++) = (*loc++) & (*oter++);
        }
        return ret;
    }

    IBigInteger operator|(const IBigInteger& other) {
        IBigInteger ret;
        u8* rp = (u8*) &ret.mData;
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*rp++) = (*loc++) | (*oter++);
        }
        return ret;
    }

    IBigInteger operator^(const IBigInteger& other) {
        IBigInteger ret;
        u8* rp = (u8*) &ret.mData;
        u8* loc = (u8*) &mData;
        const u8* oter = (u8*) &other.mData;
        for(u32 i = 0; i < sizeof(T); ++i) {
            (*rp++) = (*loc++) ^ (*oter++);
        }
        return ret;
    }

    u8* operator&() {
        return (u8*) &mData;
    }

    /**
    *@brief usefull for bytes endian transform.
    */
    u8* swapByte() {
        u8* start = (u8*) &mData;
        u8* end = start + sizeof(T) - 1;
        u8 tmp;
        for(; start < end;) {
            tmp = *start;
            *start++ = *end;
            *end-- = tmp;
        }
        return (u8*) &mData;
    }

private:
    T mData;
};

} //namespace irr

#endif //APP_IBIGINTEGER_H
