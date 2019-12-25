/*
 * File:   CFileLogReceiver.h
 * Author: antmuse
 *
 * Created on 2010年11月26日, 下午10:23
 */
#ifndef APP_CFILELOGRECEIVER_H
#define	APP_CFILELOGRECEIVER_H

#include "HConfig.h"
#include "IAntLogReceiver.h"
#include "CFileWriter.h"


namespace irr {

class CFileLogReceiver : public IAntLogReceiver {
public:
    CFileLogReceiver();
    ~CFileLogReceiver();

    //bool log(const c8* sender, const fschar_t* message, ELogLevel level);
    virtual bool log(ELogLevel iLevel, const c8* timestr, const c8* iSender, const c8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* timestr, const wchar_t* iSender, const wchar_t* iMessage)override;

private:
    io::CFileWriter mFile;
};

}//namespace irr 

#endif	/* APP_CFILELOGRECEIVER_H */

