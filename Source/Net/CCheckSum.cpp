#include "CCheckSum.h"

namespace irr {

void CCheckSum::add(u32 value) {
    union {
        u32 value;
        u8 bytes[4];
    }data;

    data.value = value;

    for(u32 i = 0; i < sizeof(data.bytes); i++) {
        add(data.bytes[i]);
    }
}


void CCheckSum::add(u16 value) {
    union {
        u16 value;
        u8 bytes[2];
    }data;

    data.value = value;

    for(u32 i = 0; i < sizeof(data.bytes); i++) {
        add(data.bytes[i]);
    }
}


void CCheckSum::add(u8 value) {
    u8 cipher = (u8) (value ^ (r >> 8));
    r = (cipher + r) * c1 + c2;
    mSum += cipher;
}


void CCheckSum::add(u8* buffer, u32 length) {
    for(u32 i = 0; i < length; i++) {
        add(buffer[i]);
    }
}

}// namespace irr
