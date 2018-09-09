#ifndef APP_CNETCLIENTHTTP_H
#define APP_CNETCLIENTHTTP_H


#include "INetClientSeniorTCP.h"
#include "irrString.h"
#include "CNetAddress.h"
#include "CNetHttpRequest.h"
#include "CNetHttpResponse.h"

namespace irr {
namespace net {
class INetEventerHttp;

class CNetClientHttp : public INetEventer {
public:
    CNetClientHttp(INetClientSeniorTCP* hub);

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

    virtual s32 onEvent(SNetEvent& it)override;


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
    INetClientSeniorTCP* mHub;
};


}// end namespace net
}// end namespace irr

#endif //APP_CNETCLIENTHTTP_H
