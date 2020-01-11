#ifndef APP_CNETUTILITY_H
#define APP_CNETUTILITY_H

#include "HNetConfig.h"

namespace irr {
namespace net {

class CNetUtility {
public:
    /**
    *@brief Load the socket lib, windows only, else useless.
    *@return 0 if successed, else failed.
    */
    static s32 loadSocketLib();

    /**
    *@brief Unload the socket lib, windows only, else useless.
    *@return 0 if successed, else failed.
    */
    static s32 unloadSocketLib();

    static s32 getSocketError();

    static s32 getLastError();

    static void setLastError(s32 it);

private:
    CNetUtility() {
    };

    ~CNetUtility() {
    };
};


}// end namespace net
}// end namespace irr


#endif //APP_CNETUTILITY_H