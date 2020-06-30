#ifndef APP_CENCODERMD5_H
#define APP_CENCODERMD5_H

#include "HConfig.h"

namespace app {

class CEncoderMD5 {
public:
    const static u8 APP_MARK_LENGTH = 16;

    struct SMD5_Context {
        u32 lo, hi;
        u32 a, b, c, d;
        u8 buffer[64];
        u32 block[16];
    };

    static void initialize(CEncoderMD5::SMD5_Context* ctx);

    static void update(CEncoderMD5::SMD5_Context* ctx, const void* data, u32 size);

    static void finish(CEncoderMD5::SMD5_Context* ctx, u8* result);


    CEncoderMD5() {
        CEncoderMD5::initialize(&mContext);
    }

    ~CEncoderMD5() {
    }

    APP_FORCE_INLINE void update(const void* iData, u32 iSize) {
        CEncoderMD5::update(&mContext, iData, iSize);
    }

    APP_FORCE_INLINE u8* finish() {
        CEncoderMD5::finish(&mContext, mResult);
        return mResult;
    }

    ///return A 16 bytes result
    APP_FORCE_INLINE const u8* getResult() const {
        return mResult;
    }

    ///reinitialized.
    APP_FORCE_INLINE void reset() {
        CEncoderMD5::initialize(&mContext);
    }

    APP_FORCE_INLINE bool operator==(const CEncoderMD5& other) const {
        return other == mResult;
    }

    bool operator==(const u8* const other) const;

private:
    static const void* body(CEncoderMD5::SMD5_Context* ctx, const void* data, u32 size);
    SMD5_Context mContext;
    u8 mResult[APP_MARK_LENGTH];
};


}// end namespace app
#endif //APP_CENCODERMD5_H
