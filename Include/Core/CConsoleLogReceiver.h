/*
 * File:   CConsoleLogReceiver.h
 * Author: antmuse
 *
 * Created on 2010年11月26日, 下午10:21
 */

#ifndef ANTMUSE_CCONSOLELOGRECEIVER_H
#define	ANTMUSE_CCONSOLELOGRECEIVER_H

#include "HConfig.h"
#include "IAntLogReceiver.h"



namespace irr {

class CConsoleLogReceiver : public IAntLogReceiver {
public:
    CConsoleLogReceiver();
    ~CConsoleLogReceiver();
    virtual bool log(ELogLevel iLevel, const c8* iSender, const c8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMessage)override;
};


}//namespace irr 

#endif	/* ANTMUSE_CCONSOLELOGRECEIVER_H */

