#ifndef APP_CNETCLIENTHTTP_H
#define APP_CNETCLIENTHTTP_H


#include "CNetService.h"
#include "irrString.h"
#include "CNetAddress.h"
#include "CNetHttpRequest.h"
#include "CNetHttpResponse.h"

namespace irr {
namespace net {
class INetEventerHttp;

class CNetClientHttp : public INetEventer {
public:
    CNetClientHttp(CNetServiceTCP* hub);

    virtual ~CNetClientHttp();

    void setURL(const core::stringc& url, bool updateIP) {
        mRequest.getURL().set(url);
        mAddressRemote.setPort(mRequest.getURL().getPort());
        if(updateIP) {
            mAddressRemote.setDomain(mRequest.getURL().getHost().c_str());
        }
    }

    CNetHttpRequest& getRequest() {
        return mRequest;
    }

    bool request();

    void setNetEventer(INetEventerHttp* it) {
        mReceiver = it;
    }

    bool start();

    bool stop();

    CNetAddress& getRemoteAddress() {
        return mAddressRemote;
    }

    const CNetAddress& getRemoteAddress() const {
        return mAddressRemote;
    }

    virtual s32 onConnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;
    virtual s32 onDisconnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;
    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;
    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

private:
    void clear();

    void postData();

    void onBody();

    bool mRelocation; ///<web host relocation.
    u32 mSession;
    INetEventerHttp* mReceiver;
    CNetHttpResponse mResponse;
    CNetHttpRequest mRequest;
    CNetAddress mAddressRemote;
    CNetServiceTCP* mHub;
};


}// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTHTTP_H
