#ifndef APP_INETSESSION_H
#define	APP_INETSESSION_H

#include "INetEventer.h"

namespace irr {
namespace net {
class CNetSocket;
class CNetAddress;




class INetSession {
public:
    INetSession() {
    }

    virtual ~INetSession() {
    }

    virtual const CNetSocket& getSocket() = 0;

    virtual void setEventer(INetEventer* it) = 0;

    virtual INetEventer* getEventer()const = 0;

    virtual s32 send(u32 id, const void* iBuffer, s32 iSize) = 0;

    virtual bool connect(u32 id, const CNetAddress& it) = 0;

    virtual bool disconnect(u32 id) = 0;
};


} //namespace net
} //namespace irr

#endif //APP_INETSESSION_H
