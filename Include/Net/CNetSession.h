#ifndef APP_CNETSESSION_H
#define	APP_CNETSESSION_H

#include "INetSession.h"
#include "SClientContext.h"
#include "CTimerWheel.h"


namespace irr {
class CEventPoller;
namespace net {
//class CNetClientSeniorTCP;

enum ENetUserCommand {
    ENET_CMD_CONNECT = 0x00100000U,
    ENET_CMD_DISCONNECT = 0x00200000U,
    ENET_CMD_TIMEOUT = 0x00400000U,
    ENET_CMD_SEND = 0x00800000U,
    ENET_CMD_NEW_SESSION = 0x00300000U,
    ENET_CMD_MASK = 0xFFF00000U,
    ENET_SESSION_MASK = 0x000FFFFFU
};


class CNetSession : public INetSession {
public:

    CNetSession();

    virtual ~CNetSession();

    APP_FORCE_INLINE u32 getMask(u32 index, u32 level) {
        return (ENET_SESSION_MASK&index) | (ENET_CMD_MASK& (level << 20));
    }

    APP_FORCE_INLINE void setIndex(u32 pos) {
        APP_ASSERT(pos <= ENET_SESSION_MASK);
        mMask = (mMask & ENET_CMD_MASK) | (pos & ENET_SESSION_MASK);
    }

    /*APP_FORCE_INLINE u32 getLevel()const {
        return (mMask & 0xFF000000U);
    }*/

    APP_FORCE_INLINE u32 getIndex()const {
        return (mMask & ENET_SESSION_MASK);
    }

    APP_FORCE_INLINE u32 getID()const {
        return mID;
    }

    APP_FORCE_INLINE bool isValid()const {
        return mID == mMask;
    }

    SNetAddress& getRemoteAddress() {
        return mAddressRemote;
    }

    SNetAddress& getLocalAddress() {
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

    virtual s32 send(const c8* iBuffer, s32 iSize)override;
    virtual bool connect(const SNetAddress& it)override;
    virtual bool disconnect()override;

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
        u32 level = (mMask & ENET_CMD_MASK);
        level += 0x00100000U;
        if(0 == level) {//level can not be zero
            level += 0x00100000U;
        }
        mMask = (mMask & ENET_SESSION_MASK) | level;
    }

    void postEvent(ENetEventType iEvent);

    CTimerWheel::STimeNode mTimeNode;
    u32 mMask;    //12bits is level, 20bit is index
    u32 mID;      //last level | index
    u32 mStatus;
    s32 mCount;
    u64 mTime;
    CNetSocket mSocket;
    INetEventer* mEventer;
    CEventPoller* mPoller;
    //CNetClientSeniorTCP* mSessionHub;
    //void* mFunctionConnect;             ///<Windows only
    //void* mFunctionDisonnect;           ///<Windows only
    SContextIO mActionSend;
    SContextIO mActionReceive;
    SContextIO mActionConnect;
    SContextIO mActionDisconnect;
    CNetPacket mPacketSend;
    CNetPacket mPacketReceive;
    SNetAddress mAddressRemote;
    SNetAddress mAddressLocal;
};


} //namespace net
} //namespace irr

#endif //APP_CNETSESSION_H
