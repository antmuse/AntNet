#ifndef APP_CNETSESSIONPOOL_H
#define	APP_CNETSESSIONPOOL_H

#include "CNetSession.h"

namespace app {
namespace net {

class CNetSessionPool {
public:

    CNetSessionPool();

    ~CNetSessionPool();

    CNetSession* getIdleSession(u64 now_ms, u32 timeinterval_ms);

    void addIdleSession(CNetSession* it);

    void addIdleSession(u32 idx) {
        addIdleSession(mAllContext[idx]);
    }

    CNetSession& operator[](u32 idx)const {
        return *(mAllContext[idx]);
    }

    CNetSession* getSession(u32 idx)const {
        return mAllContext[idx];
    }

    CNetSession* getSession(u64 idx)const {
        return mAllContext[idx];
    }

    void create(u32 max);

    u32 getMaxContext()const {
        return mMax;
    }

    u32 getIdleCount()const {
        return mIdle;
    }
    u32 getActiveCount()const {
        return mMax - mIdle;
    }

    bool waitClose();



private:
    volatile bool mClosed;
    u32 mMax;
    u32 mIdle;
    CNetSession** mAllContext;

    //idle list
    CNetSession* mHead;
    CNetSession* mTail;

    void clearAll();
    CNetSession* createSession();
    CNetSessionPool(const CNetSessionPool& it) = delete;
    CNetSessionPool& operator=(const CNetSessionPool& it) = delete;
};


} //namespace net
} //namespace app

#endif //APP_CNETSESSIONPOOL_H
