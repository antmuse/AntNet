#ifndef APP_IREFERENCECOUNTED_H
#define APP_IREFERENCECOUNTED_H

#include "HConfig.h"

namespace app {

class IReferenceCounted {
public:
    /// Constructor.
    IReferenceCounted() : mCounter(1) {
    }

    /// Destructor.
    virtual ~IReferenceCounted() {
    }

    void grab() const {
        ++mCounter;
    }

    bool drop() const {
        // someone is doing bad reference counting.
        APP_ASSERT(mCounter > 0);

        --mCounter;
        if (!mCounter) {
            delete this;
            return true;
        }

        return false;
    }

    /**
    * @brief Get the reference count.
    * @return Current value of the reference counter.
    */
    s32 getReferenceCount() const {
        return mCounter;
    }

private:
    /// The reference counter. Mutable to do reference counting on const objects.
    mutable s32 mCounter;
};

} // end namespace app

#endif //APP_IREFERENCECOUNTED_H
