/*
 * File:   CHtmlLogReceiver.h
 * Author: antmuse@live.cn
 *
 * Created on 2019年12月24日
 */
#ifndef APP_CHTMLLOGRECEIVER_H
#define	APP_CHTMLLOGRECEIVER_H

#include "ILogReceiver.h"
#include "CFileWriter.h"


namespace app {

class CHtmlLogReceiver : public ILogReceiver {
public:
    CHtmlLogReceiver();
    ~CHtmlLogReceiver();

    //bool log(const s8* sender, const tchar* message, ELogLevel level);
    virtual bool log(ELogLevel iLevel, const s8* timestr, const s8* iSender, const s8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* timestr, const wchar_t* iSender, const wchar_t* iMessage)override;

private:
    void writeHead();
    io::CFileWriter mFile;
};

}//namespace app 

#endif	//APP_CHTMLLOGRECEIVER_H

