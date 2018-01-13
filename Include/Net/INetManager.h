#ifndef APP_INETMANAGER_H
#define APP_INETMANAGER_H

#include "INetServer.h"
#include "INetClient.h"
#include "INetClientSeniorTCP.h"

namespace irr {
namespace net {

class INetManager {
public:
    virtual INetServer* createServer(ENetNodeType type) = 0;

    virtual INetClient* createClient(ENetNodeType type) = 0;

    virtual INetClientSeniorTCP* createClientSeniorTCP() = 0;

    virtual INetClient* addClient(ENetNodeType type) = 0;

    virtual INetClient* getClientTCP(u32 index) = 0;

    virtual INetClient* getClientUDP(u32 index) = 0;

    virtual bool start() = 0;

    virtual bool stop() = 0;

protected:
    INetManager() {
    }

    virtual ~INetManager() {
    }
};



/**
*@brief Get the unique net manager.
*@return Net manager
*/
INetManager* AppGetNetManagerInstance();


}// end namespace net
}// end namespace irr

#endif //APP_INETMANAGER_H
