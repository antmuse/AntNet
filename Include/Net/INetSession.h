#ifndef APP_INETSESSION_H
#define	APP_INETSESSION_H

#include "INetEventer.h"

namespace irr {
namespace net {
class CNetSocket;
struct SNetAddress;

class INetSession {
public:
    INetSession() {
    }

    virtual ~INetSession() {
    }

    virtual const CNetSocket& getSocket() = 0;

    virtual void setEventer(INetEventer* it) = 0;

    virtual INetEventer* getEventer()const = 0;

    virtual s32 send(const c8* iBuffer, s32 iSize) = 0;

    virtual bool connect(const SNetAddress& it) = 0;

    virtual bool disconnect() = 0;
};


} //namespace net
} //namespace irr

#endif //APP_INETSESSION_H
