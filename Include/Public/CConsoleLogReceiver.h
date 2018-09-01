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


#ifdef APP_COMPILE_WITH_CONSOLE_LOG_RECEIVER

namespace irr {

class CConsoleLogReceiver : public IAntLogReceiver {
public:
	CConsoleLogReceiver();
	~CConsoleLogReceiver();
	virtual bool log(ELogLevel iLevel, const c8* iSender, const c8* iMessage);
	virtual bool log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMessage);
};


}//namespace irr 

#endif //APP_COMPILE_WITH_CONSOLE_LOG_RECEIVER

#endif	/* ANTMUSE_CCONSOLELOGRECEIVER_H */

