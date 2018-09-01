#ifndef APP_CNETMANAGER_H
#define APP_CNETMANAGER_H

#include "INetManager.h"
#include "IRunnable.h"
#include "irrArray.h"
#include "CMutex.h"

namespace irr {
class CThread;

namespace net {

class CNetManager : public IRunnable, public INetManager {
public:
    static INetManager* getInstance();

    virtual INetServer* createServer(ENetNodeType type)override;

    virtual INetClient* createClient(ENetNodeType type)override;

    //virtual INetClientHttp* createClientHttp()override;
    virtual INetClientSeniorTCP* createClientSeniorTCP(CNetConfig* cfg)override;

    virtual INetClient* addClient(ENetNodeType type)override;

    virtual INetClient* getClientTCP(u32 index)override;

    virtual INetClient* getClientUDP(u32 index)override;

    virtual void run() override;

    virtual bool start()override;

    virtual bool stop()override;

protected:
    void stopAll();

    bool mRunning;
    core::array<INetClient*> mAllConnectionTCP;
    core::array<INetClient*> mAllConnectionUDP;
    core::array<INetServer*> mAllServerTCP;
    core::array<INetServer*> mAllServerUDP;
    CMutex mMutex;
    CThread* mThread;

private:
    CNetManager();
    virtual ~CNetManager();
};

}// end namespace net
}// end namespace irr

#endif //APP_CNETMANAGER_H
