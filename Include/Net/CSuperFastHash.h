#ifndef APP_CSUPERFASTHASH_H
#define APP_CSUPERFASTHASH_H

#include "irrTypes.h"

namespace irr {

u32 SuperFastHash (const c8* data, s32 length);
u32 SuperFastHashIncremental (const c8* data, s32 len, u32 lastHash);
u32 SuperFastHashFile (const c8* filename);
u32 SuperFastHashFilePtr (FILE *fp);

}//namespace irr
#endif //APP_CSUPERFASTHASH_H
