#ifndef APP_INETCLIENTSENIORTCP_H
#define APP_INETCLIENTSENIORTCP_H

//#include "HConfig.h"
//#include "INetEventer.h"
#include "IRunnable.h"
#include "INetSession.h"

namespace irr {
namespace net {


class INetClientSeniorTCP : public IRunnable {
public:
    INetClientSeniorTCP() {
    }

    virtual ~INetClientSeniorTCP() {
    }

    virtual u64 getTotalTime() const = 0;

    virtual u64 getTotalReceived() const = 0;

    virtual void setTimeoutInterval(u32 it) = 0;


    virtual u32 getTimeoutInterval()const = 0;

    /**
     *@brief Get socket reused rate, in percent.
     *@return Reuse rate, in percent.
     */
    virtual u32 getReuseRate() = 0;

    /**
     *@brief Get how many session created.
     *@return Count of created session.
     */
    virtual u32 getTotalSession() = 0;

    /**
     *@brief Get how many socket created.
     *@return Count of created socket.
     */
    virtual u32 getCreatedSocket() = 0;

    /**
     *@brief Get how many socket closed.
     *@return Count of closed socket.
     */
    virtual u32 getClosedSocket() = 0;

    virtual bool start() = 0;

    virtual bool stop() = 0;

    /**
     *@brief Get a net session.
     *@param receiver The event receiver of net session.
     *@return A valid session if success, else 0.
     */
    virtual u32 getSession(INetEventer* receiver) = 0;

    virtual s32 send(u32 id, const void* buffer, s32 size) = 0;

    virtual bool connect(u32 id, const CNetAddress& it) = 0;

    virtual void setEventer(u32 id, INetEventer* it) =0;

    virtual bool disconnect(u32 id) = 0;

};


}// end namespace net
}// end namespace irr

#endif //APP_INETCLIENTSENIORTCP_H
