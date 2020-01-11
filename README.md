AntNet V0.0.0.5
====
A cross-platform net lib for Windows&amp;Linux&amp;Android.
# Usage
----

```cpp
//demo 1: echo-server
void AppRunEchoServer() {
    net::CNetConfig* config = new net::CNetConfig();
    net::CNetAddress addr(9981);
    net::CNetEchoServer evt;
    net::CNetServerAcceptor accpetor(config);
    config->drop();
    evt.setServer(&accpetor);
    accpetor.setLocalAddress(addr);
    accpetor.start();
    irr::c8 key = '\0';
    while('*' != key) {
        printf("@Please input [*] to quit\n");
        scanf("%c", &key);
    }
    accpetor.stop();
}
```
