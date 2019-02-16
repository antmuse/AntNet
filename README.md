AntNet V0.0.0.2
====
A cross-platform net lib, current for Windows&amp;Linux&amp;Android.
# Usage
----

```cpp
//wait for more
void AppStartServer() {
    net::CNetConfig* config = new net::CNetConfig();
    config->mReuse = true;
    config->mMaxPostAccept = 8;
    config->mMaxFetchEvents = 28;
    config->mMaxContext = 200;
    config->mPollTimeout = 1000;
    config->mSessionTimeout = 30000;
    config->mMaxWorkThread = 2;
    config->check();
    config->print();

    net::CDefaultNetEventer evt;
    net::CNetServerAcceptor accpetor(config);
    config->drop();
    evt.setServer(&accpetor);
    net::CNetAddress addr(9981);
    accpetor.setLocalAddress(addr);
    accpetor.setEventer(&evt);
    accpetor.start();
    AppQuit();
    accpetor.stop();
}

class CDefaultNetEventer : public INetEventer {
    public:
        CDefaultNetEventer();

        virtual ~CDefaultNetEventer();

        virtual s32 onConnect(u32 sessionID,
                const CNetAddress& local, const CNetAddress& remote)override;

        virtual s32 onDisconnect(u32 sessionID,
                const CNetAddress& local, const CNetAddress& remote)override;

        virtual s32 onSend(u32 sessionID, void* buffer, s32 size, s32 result)override;

        virtual s32 onReceive(u32 sessionID, void* buffer, s32 size)override;

        virtual s32 onLink(u32 sessionID,
                const CNetAddress& local, const CNetAddress& remote)override;

    private:
        u32 mSession;
};
```
