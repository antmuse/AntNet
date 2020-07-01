/*
 * File:   CFileLogReceiver.h
 * Author: antmuse
 *
 * Created on 2010年11月26日, 下午10:23
 */
#ifndef APP_CFILELOGRECEIVER_H
#define	APP_CFILELOGRECEIVER_H

#include "ILogReceiver.h"
#include "CFileWriter.h"


namespace app {

class CFileLogReceiver : public ILogReceiver {
public:
    CFileLogReceiver();
    ~CFileLogReceiver();

    //bool log(const s8* sender, const tchar* message, ELogLevel level);
    virtual bool log(ELogLevel iLevel, const s8* timestr, const s8* iSender, const s8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* timestr, const wchar_t* iSender, const wchar_t* iMessage)override;

private:
    io::CFileWriter mFile;
};

}//namespace app 

#endif	/* APP_CFILELOGRECEIVER_H */

