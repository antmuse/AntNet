/*
 * File:   CHtmlLogReceiver.h
 * Author: antmuse@live.cn
 *
 * Created on 2019年12月24日
 */
#ifndef APP_CHTMLLOGRECEIVER_H
#define	APP_CHTMLLOGRECEIVER_H

#include "HConfig.h"
#include "IAntLogReceiver.h"
#include "CFileWriter.h"


namespace irr {

class CHtmlLogReceiver : public IAntLogReceiver {
public:
    CHtmlLogReceiver();
    ~CHtmlLogReceiver();

    //bool log(const c8* sender, const fschar_t* message, ELogLevel level);
    virtual bool log(ELogLevel iLevel, const c8* timestr, const c8* iSender, const c8* iMessage)override;
    virtual bool log(ELogLevel iLevel, const wchar_t* timestr, const wchar_t* iSender, const wchar_t* iMessage)override;

private:
    void writeHead();
    io::CFileWriter mFile;
};

}//namespace irr 

#endif	//APP_CHTMLLOGRECEIVER_H

