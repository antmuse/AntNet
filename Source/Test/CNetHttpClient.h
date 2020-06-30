#ifndef APP_CNETHTTPCLIENT_H
#define APP_CNETHTTPCLIENT_H

#include "http_parser.h"
#include "AppList.h"
#include "INetEventer.h"
#include "CNetPacket.h"
#include "CNetHttpURL.h"

namespace app {
namespace net {
class CNetServiceTCP;

class CNetHttpClient : public INetEventer {
public:
    CNetHttpClient();

    virtual ~CNetHttpClient();

    void setHub(CNetServiceTCP* hub) {
        mServer = mServer ? mServer : hub;
    }

    bool request(const s8* url);

    virtual INetEventer* onAccept(const CNetAddress& local)override {
        return this;
    }

    virtual s32 onTimeout(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onConnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onDisconnect(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

    virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

    virtual s32 onReceive(const CNetAddress& remote, u32 sessionID, void* buffer, s32 size)override;

    virtual s32 onLink(u32 sessionID,
        const CNetAddress& local, const CNetAddress& remote)override;

private:
    CNetServiceTCP* mServer;
    CNetPacket mPack;
    http_parser_settings mSets;
    http_parser mParser;
    http_parser_type mParserType;
    CNetHttpURL mURL;
};

}//namespace net
}//namespace app

#endif //APP_CNETHTTPCLIENT_H