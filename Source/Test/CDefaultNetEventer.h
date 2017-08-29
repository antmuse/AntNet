#ifndef APP_CDEFAULTNETEVENTER_H
#define APP_CDEFAULTNETEVENTER_H

#include "INetEventer.h"

namespace irr{
    namespace net{
        //for net test
        class CDefaultNetEventer : public INetEventer {
        public:
            CDefaultNetEventer();
            virtual ~CDefaultNetEventer();
            virtual void onReceive(CNetPacket& pack);
        };

    }//namespace net{
}//namespace irr{

#endif //APP_CDEFAULTNETEVENTER_H