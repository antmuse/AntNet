#ifndef APP_CNETMANAGER_H
#define APP_CNETMANAGER_H

#include "IRunnable.h"
#include "irrArray.h"
#include "CMutex.h"
#include "INetClient.h"
#include "INetServer.h"

namespace irr {
class CThread;

namespace net {

class CNetManager : public IRunnable {
public:
    static CNetManager& getInstance();

    virtual void run() override;

    bool start();

    bool stop();

    INetServer* createServer(ENetNodeType type, INetEventer* iEvt);

    INetClient* createClient(ENetNodeType type, INetEventer* iEvt);

    INetClient* addClient(ENetNodeType type, INetEventer* iEvt);

    INetClient* getClientTCP(u32 index);

    INetClient* getClientUDP(u32 index);


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

    CNetManager(const CNetManager& it) = delete;
    CNetManager& operator=(const CNetManager& it) = delete;
};

}// end namespace net
}// end namespace irr

#endif //APP_CNETMANAGER_H
