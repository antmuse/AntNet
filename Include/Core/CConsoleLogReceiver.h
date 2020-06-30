/*
 * File:   CConsoleLogReceiver.h
 * Author: antmuse
 *
 * Created on 2010年11月26日, 下午10:21
 */

#ifndef ANTMUSE_CCONSOLELOGRECEIVER_H
#define	ANTMUSE_CCONSOLELOGRECEIVER_H

#include "ILogReceiver.h"

namespace app {

class CConsoleLogReceiver : public ILogReceiver {
public:
    CConsoleLogReceiver();
    ~CConsoleLogReceiver();
    virtual bool log(ELogLevel iLevel, const s8* timestr, const s8* iSender, const s8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* timestr, const wchar_t* iSender, const wchar_t* iMessage)override;
};


}//namespace app 

#endif	/* ANTMUSE_CCONSOLELOGRECEIVER_H */

