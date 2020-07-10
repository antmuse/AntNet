#include "CHtmlLogReceiver.h"
#include <wchar.h>

namespace app {

CHtmlLogReceiver::CHtmlLogReceiver() : ILogReceiver() {
    core::CPath fn(APP_STR("App.html"));
    mFile.openFile(fn, true);
    if(0 == mFile.getFileSize()) {
        writeHead();
    }
}

CHtmlLogReceiver::~CHtmlLogReceiver() {
}

void CHtmlLogReceiver::writeHead() {
    const s8* head =
        "<html>\n" \
        "<head>\n" \
        "<meta http-equiv=\"Content-Type\" content=\"text/html charset=utf-8\" />\n" \
        "<title>Log page</title>\n" \
        "<style type=\"text/css\">\n" \
        "body, html {\n" \
        "background: #000000;\n" \
        "width: 1400px;\n" \
        "font-family: Arial;\n" \
        "font-size: 16px;\n" \
        "color: #C0C0C0;\n" \
        "}\n" \
        "h1 {\n" \
        "color : #FFFFFF;\n" \
        "border-bottom : 1px dotted #888888;\n" \
        "}\n" \
        "pre {\n" \
        "font-family : arial;\n" \
        "margin : 0;\n" \
        "}\n" \
        ".box {\n" \
        "border : 1px dotted #818286;\n" \
        "padding : 5px;\n" \
        "margin: 5px;\n" \
        "width: 1400px;\n" \
        "background-color : #292929;\n" \
        "}\n" \
        ".Error {\n" \
        "color: #EE1100;\n" \
        "font-weight: bold;\n" \
        "}\n" \
        ".Warn {\n" \
        "color: #FFCC00;\n" \
        "font-weight: bold;\n" \
        "}\n" \
        ".Critical {\n" \
        "color: #BB0077;\n" \
        "font-weight: bold;\n" \
        "}\n" \
        ".Info {\n" \
        "color: #C0C0C0;\n" \
        "}\n" \
        ".Debug {\n" \
        "color: #CCA0A0;\n" \
        "}\n" \
        "</style>\n" \
        "</head>\n\n" \
        "<body>\n" \
        "<h1>Antmuse Log</h1>\n" \
        "<h3>2.0.0</h3>\n" \
        "<div class=\"box\">\n" \
        "<table>\n";
    mFile.write(head, strlen(head));
}

bool CHtmlLogReceiver::log(ELogLevel level, const wchar_t* timestr, const wchar_t* pSender, const wchar_t* pMessage) {
    wchar_t wcache[1024];
    swprintf(wcache, 1024, L"[%s][%s] %s> %s\n", timestr, AppWLogLevelNames[level], pSender, pMessage);
    return true;
}


bool CHtmlLogReceiver::log(ELogLevel level, const s8* timestr, const s8* sender, const s8* message) {
    mFile.write("<tr>\n<td width=\"160\">", sizeof("<tr>\n<td width=\"160\">") - 1);
    mFile.write(timestr, strlen(timestr));
    mFile.write("</td>\n<td class=\"", sizeof("</td>\n<td class=\"") - 1);
    mFile.write(AppLogLevelNames[level], strlen(AppLogLevelNames[level]));
    mFile.write("\"><pre>\n", sizeof("\"><pre>\n") - 1);
    mFile.writeParams("%s>%s", sender, message);
    mFile.write("\n</pre></td>\n</tr>\n", sizeof("\n</pre></td>\n</tr>\n") - 1);
    return true;
}

}//namespace app

