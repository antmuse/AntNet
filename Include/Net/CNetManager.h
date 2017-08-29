#ifndef APP_CNETMANAGER_H
#define APP_CNETMANAGER_H

#include "INetManager.h"
#include "IRunnable.h"
#include "irrArray.h"
#include "CMutex.h"

namespace irr {
    class CThread;

    namespace net {

        class CNetManager : public IRunnable, public INetManager{
        public:
            static INetManager* getInstance();

            virtual INetServer* createServerNormalTCP();

            virtual INetServer* createServerNormalUDP();

            virtual INetServer* createServerSeniorTCP();

            virtual INetServer* createServerSeniorUDP();

            virtual INetClient* addClientTCP();

            virtual INetClient* addClientUDP();

            virtual INetClient* getClientTCP(u32 index);

            virtual INetClient* getClientUDP(u32 index);

            //override
            virtual void run();

            virtual bool start();

            virtual bool stop();

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
