#include "CDefaultNetEventer.h"
#include "IAppLogger.h"
#include "CNetPacket.h"

namespace irr{
    namespace net{

        CDefaultNetEventer::CDefaultNetEventer(){
        }


        CDefaultNetEventer::~CDefaultNetEventer(){
        }


        void CDefaultNetEventer::onReceive(net::CNetPacket& pack){
            core::array<c8> items;
            pack.seek(0); 
            u8 typeID = pack.readU8();
            switch(typeID){
            case net::ENET_DATA:
                pack.readString(items);
                IAppLogger::log(ELOG_INFO, "CDefaultNetEventer::onReceive",  "[%s]", items.pointer());
                break;
            default:  break;
            }//switch
            IAppLogger::log(ELOG_INFO,"CDefaultNetEventer::onReceive", "=======================================");
        }


    }//namespace net{
}//namespace irr{