#ifndef APP_CNETSESSION_H
#define	APP_CNETSESSION_H

#include "INetSession.h"
#include "SClientContext.h"
#include "CNetPacket.h"
#include "CTimerWheel.h"


namespace irr {
class CEventPoller;
namespace net {
//class CNetClientSeniorTCP;

enum ENetUserCommand {
    ENET_CMD_CONNECT = 0x00010000U,
    ENET_CMD_DISCONNECT = 0x00020000U,
    ENET_CMD_TIMEOUT = 0x00040000U,
    ENET_CMD_SEND = 0x00080000U,
    ENET_CMD_NEW_SESSION = 0x00030000U,
    ENET_CMD_MASK = 0xFFFF0000U,

    ENET_SESSION_BITS = 16,
    ENET_SERVER_BITS = 8,
    ENET_SESSION_MASK = 0x0000FFFFU,
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

    void setNext(CNetSession* it) {
        mNext = it;
    }

    CNetSession* getNext()const {
        return mNext;
    }

    APP_FORCE_INLINE u32 getMask(u32 index, u32 level)const {
        return (ENET_SESSION_MASK & index) | (level << (ENET_SESSION_BITS+ ENET_SERVER_BITS));
    }

    APP_FORCE_INLINE void setHub(u32 hid) {
        mMask |= (ENET_SERVER_MASK & (hid << ENET_SESSION_BITS));
    }

    APP_FORCE_INLINE void setIndex(u32 pos) {
        APP_ASSERT(pos <= ENET_SESSION_MASK);
        mMask = (mMask & ENET_CMD_MASK) | (pos & ENET_SESSION_MASK);
    }

    APP_FORCE_INLINE u32 getLevel(u32 id)const {
        return (mMask & ENET_SESSION_LEVEL_MASK);
    }

    APP_FORCE_INLINE u32 getIndex()const {
        return (mMask & ENET_SESSION_MASK);
    }

    APP_FORCE_INLINE u32 getID()const {
        return mMask;
    }

    APP_FORCE_INLINE bool isValid(u32 id)const {
        return id == mMask;
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

    virtual s32 send(u32 id, const void* iBuffer, s32 iSize)override;

    virtual bool connect(u32 id, const CNetAddress& it)override;

    virtual bool disconnect(u32 id)override;

    s32 postDisconnect();

    s32 postConnect();

    s32 postSend();

    s32 postReceive();

    void clear();

    void setSocket(const CNetSocket& it);

    s32 stepSend();
    s32 stepReceive();
    s32 stepConnect();
    s32 stepDisonnect();

    bool onTimeout();

    //add in time wheel
    bool onNewSession();

    void setPoller(CEventPoller* iPoller) {
        mPoller = iPoller;
    }

    CEventPoller* getPoller() {
        return mPoller;
    }

    APP_FORCE_INLINE void setTime(u64 it) {
        mTime = it;
    }

    APP_FORCE_INLINE u64 getTime()const {
        return mTime;
    }

    /*void setSessionHub(CNetClientSeniorTCP* it) {
        mSessionHub = it;
    }

    CNetClientSeniorTCP* getSessionHub() {
        return mSessionHub;
    }*/

protected:
    void upgradeLevel() {
        u32 level = (mMask & ENET_SESSION_LEVEL_MASK);
        level += 0x01000000U;
        if(0 == level) {//level can not be zero
            level += 0x01000000U;
        }
        mMask = (mMask & ENET_SESSION_LEVEL_MASK) | level;
    }

    void postEvent(ENetEventType iEvent);

    CTimerWheel::STimeNode mTimeNode;
    bool mPostedDisconnect;
    u32 mMask;    //12bits is level, 20bit is index
    u32 mStatus;
    s32 mCount;
    u64 mTime;
    CNetSocket mSocket;
    INetEventer* mEventer;
    CEventPoller* mPoller;
    CNetSession* mNext;
    //CNetClientSeniorTCP* mSessionHub;
    //void* mFunctionConnect;             ///<Windows only
    //void* mFunctionDisonnect;           ///<Windows only
    SContextIO mActionSend;
    SContextIO mActionReceive;
    SContextIO mActionConnect;
    SContextIO mActionDisconnect;
    CNetPacket mPacketSend;
    CNetPacket mPacketReceive;
    CNetAddress mAddressRemote;
    CNetAddress mAddressLocal;
};



class CNetSessionPool {
public:
    CNetSessionPool();

    ~CNetSessionPool();

    CNetSession* getIdleSession();

    void addIdleSession(CNetSession* it);

    void addIdleSession(u32 idx) {
        addIdleSession(mAllContext + idx);
    }

    CNetSession& operator[](u32 idx)const {
        return *(mAllContext + idx);
    }

    CNetSession* getSession(u32 idx)const {
        return mAllContext + idx;
    }

    CNetSession* getSession(u64 idx)const {
        return mAllContext + idx;
    }

    void create(u32 hubID, u32 max);

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
    void clearAll();


    u32 mMax;
    u32 mIdle;
    CNetSession* mAllContext;
    CNetSession* mHead;
    CNetSession* mTail;
};


} //namespace net
} //namespace irr

#endif //APP_CNETSESSION_H
