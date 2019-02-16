#include "CFileLogReceiver.h"

#include <iostream>
#include <fstream>

namespace irr {

CFileLogReceiver::CFileLogReceiver() : IAntLogReceiver() {
    firsttime = false;
}

CFileLogReceiver::~CFileLogReceiver() {
}


bool CFileLogReceiver::log(ELogLevel level, const wchar_t* pSender, const wchar_t* pMessage) {
    wchar_t wcache[1024];
    swprintf(wcache, 1024, L"[%s] %s> %s\n", AppWLogLevelNames[level], pSender, pMessage);
    return true;
}


bool CFileLogReceiver::log(ELogLevel level, const c8* sender, const c8* message) {
    std::ofstream outf;
    if(firsttime == false) {
        if(!outf.is_open()) {
            // Reset log file
            outf.setf(std::ios::fixed);
            outf.precision(3);
            outf.open("./Temp/antLog.html", std::ios::out);

            if(!outf) {
                return false;
            }

            outf << "<html>\n";
            outf << "<head>\n";
            outf << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
            outf << "<title>Antmuse log system</title>\n";
            outf << "<style type=\"text/css\">\n";

            outf << "body, html {\n";
            outf << "background: #000000;\n";
            outf << "width: 1000px;\n";
            outf << "font-family: Arial;\n";
            outf << "font-size: 16px;\n";
            outf << "color: #C0C0C0;\n";
            outf << "}\n";

            outf << "h1 {\n";
            outf << "color : #FFFFFF;\n";
            outf << "border-bottom : 1px dotted #888888;\n";
            outf << "}\n";

            outf << "pre {\n";
            outf << "font-family : arial;\n";
            outf << "margin : 0;\n";
            outf << "}\n";

            outf << ".box {\n";
            outf << "border : 1px dotted #818286;\n";
            outf << "padding : 5px;\n";
            outf << "margin: 5px;\n";
            outf << "width: 950px;\n";
            outf << "background-color : #292929;\n";
            outf << "}\n";

            outf << ".err {\n";
            outf << "color: #EE1100;\n";
            outf << "font-weight: bold\n";
            outf << "}\n";

            outf << ".warn {\n";
            outf << "color: #FFCC00;\n";
            outf << "font-weight: bold\n";
            outf << "}\n";

            outf << ".crit {\n";
            outf << "color: #BB0077;\n";
            outf << "font-weight: bold\n";
            outf << "}\n";

            outf << ".info {\n";
            outf << "color: #C0C0C0;\n";
            outf << "}\n";

            outf << ".debug {\n";
            outf << "color: #CCA0A0;\n";
            outf << "}\n";

            outf << "</style>\n";
            outf << "</head>\n\n";

            outf << "<body>\n";
            outf << "<h1>Antmuse Log</h1>\n";
            outf << "<h3>" << "2.0.0" << "</h3>\n";
            outf << "<div class=\"box\">\n";
            outf << "<table>\n";

            outf.flush();

        }
        firsttime = true;
    } else {
        outf.open("EngineLog.html", std::ios::out | std::ios::app);
        if(!outf) {
            return false;
        }
        outf << "<tr>\n";
        outf << "<td width=\"100\">";
        outf << "time";
        outf << "</td>\n";
        outf << "<td class=\"";
        switch(level) {
        case ELOG_DEBUG:
            outf << "Debug";
            break;

        case ELOG_INFO:
            outf << "Info";
            break;

        case ELOG_WARNING:
            outf << "Warn";
            break;

        case ELOG_ERROR:
            outf << "Err";
            break;

        case ELOG_CRITICAL:
            outf << "Crit";
            break;

        case ELOG_COUNT:
            outf << "Debug";
            break;

        default:
            outf << "Debug";
        }

        outf << "\"><pre>\n";
        outf << message;
        outf << "\n</pre></td>\n";
        outf << "</tr>\n";

        outf.flush();

    }
    outf.close();
    return true;
}

}//namespace irr 

