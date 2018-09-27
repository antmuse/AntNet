/*
 * File:   CFileLogReceiver.h
 * Author: antmuse
 *
 * Created on 2010年11月26日, 下午10:23
 */
#ifndef ANTMUSE_CFILELOGRECEIVER_H
#define	ANTMUSE_CFILELOGRECEIVER_H

#include "HConfig.h"
#include "IAntLogReceiver.h"


namespace irr {

class CFileLogReceiver : public IAntLogReceiver {
public:
    CFileLogReceiver();
    ~CFileLogReceiver();

    //bool log(const c8* sender, const fschar_t* message, ELogLevel level);
    virtual bool log(ELogLevel iLevel, const c8* iSender, const c8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* iSender, const wchar_t* iMessage)override;

private:
    bool firsttime;

};

}//namespace irr 

#endif	/* ANTMUSE_CFILELOGRECEIVER_H */

