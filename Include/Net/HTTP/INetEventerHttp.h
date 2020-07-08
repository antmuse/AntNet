#ifndef APP_INETEVENTERHTTP_H
#define APP_INETEVENTERHTTP_H


namespace app {
namespace net {
class CNetHttpRequest;
class CNetHttpResponse;

class INetEventerHttp {
public:
    INetEventerHttp() {
    }

    virtual ~INetEventerHttp() {
    }

    /**
    *@brief Callback function when http finished.
    *@param req The request.
    *@param val The response.
    */
    virtual void onResponse(const CNetHttpRequest& req, const CNetHttpResponse& val) = 0;
};


}// end namespace net
}// end namespace app

#endif //APP_INETEVENTERHTTP_H
