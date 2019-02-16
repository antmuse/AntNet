#ifndef APP_CNETSESSION_H
#define	APP_CNETSESSION_H

#include "HAtomicOperator.h"
#include "INetSession.h"
#include "SClientContext.h"
#include "CTimerWheel.h"
#include "CBufferQueue.h"


namespace irr {
namespace net {
class CNetServiceTCP;

enum ENetUserCommand {
    ENET_CMD_CONNECT = 0x00010000U,
    ENET_CMD_DISCONNECT = 0x00020000U,
    ENET_CMD_SEND = 0x00030000U,
    ENET_CMD_RECEIVE = 0x00040000U,
    ENET_CMD_TIMEOUT = 0x00050000U,
    ENET_CMD_MASK = 0xFFFF0000U,

    ENET_SESSION_BITS = 16,
    ENET_SERVER_BITS = 8,
    ENET_SESSION_MASK = ~ENET_CMD_MASK, //0x0000FFFFU,
    ENET_SERVER_MASK = 0x00FF0000U,
    ENET_SESSION_LEVEL_MASK = 0xFF000000U,
    ENET_ID_MASK = 0x00FFFFFFU
};

/**
*ID of net network is 32 bits:
*    16bit: used for session id.
*    8bit: used for server id.
*    8bit: used for session level.
*/
class CNetSession : public INetSession {
public:

    CNetSession();

    virtual ~CNetSession();

    void setService(CNetServiceTCP* it);

    void setNext(CNetSession* it) {
        mNext = it;
    }

    CNetSession* getNext()const {
        return mNext;
    }

    APP_FORCE_INLINE u32 getMask(u32 index, u32 level)const {
        return (ENET_SESSION_MASK & index) | (level << (ENET_SESSION_BITS + ENET_SERVER_BITS));
    }

    APP_FORCE_INLINE void setHub(u32 hid) {
        mID &= (~ENET_SERVER_MASK);
        mID |= (ENET_SERVER_MASK & (hid << ENET_SESSION_BITS));
    }

    APP_FORCE_INLINE void setIndex(u32 pos) {
        APP_ASSERT(pos <= ENET_SESSION_MASK);
        mID = (mID & ENET_CMD_MASK) | (pos & ENET_SESSION_MASK);
    }

    APP_FORCE_INLINE u32 getLevel(u32 id)const {
        return (mID & ENET_SESSION_LEVEL_MASK);
    }

    APP_FORCE_INLINE u32 getIndex()const {
        return (mID & ENET_SESSION_MASK);
    }

    APP_FORCE_INLINE u32 getID()const {
        return mID;
    }

    APP_FORCE_INLINE bool isValid(u32 id)const {
        return id == mID;
    }

    CNetAddress& getRemoteAddress() {
        return mAddressRemote;
    }

    CNetAddress& getLocalAddress() {
        return mAddressLocal;
    }

    virtual CNetSocket& getSocket() override {
        return mSocket;
    }

    virtual void setEventer(INetEventer* it)override {
        mEventer = it;
    }

    virtual INetEventer* getEventer()const override {
        return mEventer;
    }

    CTimerWheel::STimeNode& getTimeNode() {
        return mTimeNode;
    }

    bool connect(const CNetAddress& it);

    bool disconnect(u32 id);

    bool receive();

    s32 postDisconnect();
    s32 stepDisonnect();

    s32 postConnect();
    s32 stepConnect();

    /**called by IO thread
    * @param id sessionID that hold by user.
    * @param buf buf to been sent.
    * @return -1 if error, 0 if cached for send, >0 if post send now.
    */
    s32 postSend(u32 id, CBufferQueue::SBuffer* buf);

    //called by IO thread
    s32 postSend();
    s32 stepSend();

    s32 postReceive();
    s32 stepReceive();

    void postTimeout();
    s32 stepTimeout();
    s32 stepError();
    s32 stepClose();

    void clear();

    void setSocket(const CNetSocket& it);

    APP_FORCE_INLINE void setTime(u64 it) {
        mTime = it;
    }

    APP_FORCE_INLINE u64 getTime()const {
        return mTime;
    }

    s32 postEvent(ENetEventType iEvent);


    void pushGlobalQueue(CEventQueue::SNode* nd);
    bool popGlobalQueue() {
        return setInGlobalQueue(0);
    }

    void dispatchEvents();


protected:

    void upgradeLevel() {
        u32 level = (mID & ENET_SESSION_LEVEL_MASK);
        level += 1 + ENET_ID_MASK;
        if(0 == level) {//level can not be zero
            level += 1 + ENET_ID_MASK;
        }
        mID = (mID & ENET_ID_MASK) | level;
    }

    /**
    *@brief inner function
    *@param in 1=enqueue, 0=dequeu
    *@return true if need launch a event*/
    APP_INLINE bool setInGlobalQueue(s32 in) {
        const s32 cmp = in ? 0 : 1;
        return (cmp == AppAtomicFetchCompareSet(in, cmp, &mInGlobalQueue));
    }

    s32 mInGlobalQueue; //1=in global queue, 0=not in global queue.
    u32 mID;    //12bits is level, 20bit is index
    u32 mStatus;
    s32 mCount;
    u64 mTime;
    CTimerWheel::STimeNode mTimeNode;
    CNetSocket mSocket;
    INetEventer* mEventer;
    CNetSession* mNext;
    SContextIO mActionSend;
    SContextIO mActionReceive;
    SContextIO mActionConnect;
    SContextIO mActionDisconnect;
    CEventQueue::SNode* mPacketSend;
    CEventQueue::SNode* mPacketReceive;
    CEventQueue mQueueInput; //processed by IO thread
    CEventQueue mQueueEvent; //processed by worker threads
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;
    CNetServiceTCP* mService;

private:
    CNetSession(const CNetSession& it) = delete;
    CNetSession& operator=(const CNetSession& it) = delete;
};



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

private:
    CNetSessionPool(const CNetSessionPool& it) = delete;
    CNetSessionPool& operator=(const CNetSessionPool& it) = delete;

    void clearAll();
    CNetSession* createSession();

    u32 mMax;
    u32 mIdle;
    CNetSession* mAllContext[ENET_SESSION_MASK];
    CNetSession* mHead;
    CNetSession* mTail;
};


} //namespace net
} //namespace irr

#endif //APP_CNETSESSION_H
