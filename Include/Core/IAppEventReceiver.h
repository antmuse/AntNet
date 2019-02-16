#ifndef ANTMUSE_IAPPEVENTRECEIVER_H
#define	ANTMUSE_IAPPEVENTRECEIVER_H

#include "HConfig.h"
//#include "irrTypes.h"
//#include "IReferenceCounted.h"

namespace irr{

struct SAppEvent;

///Interface of an object which can receive events.
class IAppEventReceiver{
public:

    //! Destructor
    virtual ~IAppEventReceiver(){
    }

    //! Called if an event happened.
    /** Please take care that you should only return 'true' when you want to _prevent_ Engine
    * from processing the event any further. So 'true' does mean that an event is completely done.
    * Therefore your return value for all unprocessed events should be 'false'.
    \return True if the event was processed.
    */
    virtual bool onEvent(const SAppEvent& iEvent) = 0;
};


} //namespace irr
#endif	/* ANTMUSE_IAPPEVENTRECEIVER_H */

