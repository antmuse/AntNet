#ifndef APP_HNETOPERATIONTYPE_H
#define APP_HNETOPERATIONTYPE_H


namespace irr {
namespace net {

///net operation type
enum EOperationType {
    EOP_INVALID = 0,            ///<Used to init or reset
    EOP_ACCPET = 1,             ///<Accept operation
    EOP_SEND = 1 << 1,          ///<Send operation
    EOP_RECEIVE = 1 << 2,       ///<Receive operation
    EOP_CONNECT = 1 << 3,
    EOP_DISCONNECT = 1 << 4,
    EOP_ALL_OP = (1 << 5) - 1
};


const char* const AppOperationTypeName[] = {
    "OP_Invalid",
    "OP_Accpet",
    "OP_Send",
    "OP_Receive",
    "OP_Connect",
    "OP_Disonnect",
    0
};


}//namespace net
}//namespace irr
#endif //APP_HNETOPERATIONTYPE_H
