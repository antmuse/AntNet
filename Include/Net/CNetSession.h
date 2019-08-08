#ifndef APP_CNETSESSION_H
#define	APP_CNETSESSION_H

#include "HAtomicOperator.h"
#include "SClientContext.h"
#include "CTimerWheel.h"
#include "CBufferQueue.h"

namespace irr {
namespace net {
class CNetServiceTCP;

enum ENetUserCommand {
    ENET_CMD_CONNECT = 0x00100000U,
    ENET_CMD_DISCONNECT = 0x00200000U,
    ENET_CMD_SEND = 0x00300000U,
    ENET_CMD_RECEIVE = 0x00400000U,
    ENET_CMD_TIMEOUT = 0x00500000U,
    ENET_CMD_MASK = 0xFFF00000U,

    ENET_SESSION_BITS = 20,
    ENET_SERVER_BITS = 4,
    ENET_SESSION_MASK = ~ENET_CMD_MASK, //0x000FFFFFU,
    ENET_SERVER_MASK = 0x00F00000U,
    ENET_SESSION_LEVEL_MASK = 0xFF000000U,
    ENET_ID_MASK = 0x00FFFFFFU
};

/**
*ID of net network is 32 bits:
*    16bit: used for session id.
*    8bit: used for server id.
*    8bit: used for session level.
*/
class CNetSession {
public:
    CNetSession();

    ~CNetSession();

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

    const CNetAddress& getRemoteAddress()const {
        return mAddressRemote;
    }

    const CNetAddress& getLocalAddress()const {
        return mAddressLocal;
    }

    void setRemoteAddress(const CNetAddress& it) {
        mAddressRemote = it;
    }

    void setLocalAddress(const CNetAddress& it) {
        mAddressLocal = it;
    }

    void setSocket(const CNetSocket& it);

    CNetSocket& getSocket() {
        return mSocket;
    }

    void setEventer(INetEventer* it) {
        mEventer = it;
    }

    INetEventer* getEventer()const {
        return mEventer;
    }

    CTimerWheel::STimeNode& getTimeNode() {
        return mTimeNode;
    }

    s32 postDisconnect();
    s32 stepDisonnect(SContextIO& act);

    s32 postConnect();
    s32 stepConnect(SContextIO& act);

    /**called by IO thread
    * @param id sessionID that hold by user.
    * @param buf buf to been sent.
    * @return -1 if error, 0 if cached for send, >0 if post send now.
    */
    s32 postSend(u32 id, CBufferQueue::SBuffer* buf);

    //called by IO thread
    s32 postSend(CEventQueue::SNode* it);
    s32 stepSend(SContextIO& act);

    s32 postReceive();
    s32 stepReceive(SContextIO& act);

    void postTimeout();
    s32 stepTimeout();

    s32 stepClose();

    void clear();

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

    APP_FORCE_INLINE SContextIO* getAction(CEventQueue::SNode* it) const {
        APP_ASSERT(it);
        return reinterpret_cast<SContextIO*>(it + 1);
    }

    APP_FORCE_INLINE CEventQueue::SNode* getEventNode(SContextIO* it) const {
        APP_ASSERT(it);
        return reinterpret_cast<CEventQueue::SNode*>(
            reinterpret_cast<c8*>(it) - sizeof(CEventQueue::SNode));
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
    CNetSocket mSocket;
    INetEventer* mEventer;
    //CNetSession* mPrevious;
    CNetSession* mNext;
    CNetServiceTCP* mService;
    CTimerWheel::STimeNode mTimeNode;
    CEventQueue mQueueInput; //processed by IO thread
    CEventQueue mQueueEvent; //processed by worker threads
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;

private:
#if defined(APP_DEBUG)
    static s32 G_ENQUEUE_COUNT;
    static s32 G_DEQUEUE_COUNT;
#endif

    CNetSession(const CNetSession& it) = delete;
    CNetSession& operator=(const CNetSession& it) = delete;
};


} //namespace net
} //namespace irr

#endif //APP_CNETSESSION_H
