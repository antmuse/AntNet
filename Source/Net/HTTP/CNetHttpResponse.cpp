#include "CNetHttpResponse.h"
#include "fast_atof.h"
#include "irrMath.h"

#include "HNetConfig.h"
#include "IUtility.h"

#define APP_MAXSIZE_X_POWERED       128
#define APP_MAXSIZE_HTTP            (1024)
#define APP_MAXSIZE_PAGE            (1024*1024*2)
#define APP_DEFAULT_SIZE_HTTP       (256)
#define APP_DEFAULT_SIZE_PAGE       (1024*512)


namespace irr {
namespace net {

CNetHttpResponse::CNetHttpResponse() :
    mHTTP(APP_DEFAULT_SIZE_HTTP),
    mWebPage(APP_DEFAULT_SIZE_PAGE),
    mPageFull(false),
    mKeepAlive(true),
    mReadHead(true),
    mReadChunk(false),
    mReadedSize(0),
    mChunkSize(0),
    mPageSize(0) {
}


CNetHttpResponse::~CNetHttpResponse() {
}


const c8* CNetHttpResponse::import(const c8* iBuffer, u32 size) {
    static c8 lowchar[] =
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0" "0123456789\0\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    const c8* iStart = iBuffer;
    const c8* iEnd = iBuffer + size;
    while(iStart < iEnd) {
        if(mReadHead) {
            iStart = parseHead(iStart, iEnd);
            if(0 == iStart) {
                return 0;
            }
            mReadHead = false;
            mReadChunk = isByChunk();
            writeHTTP(iBuffer, u32(iStart - iBuffer));
        }//if head

        if(mReadChunk) {
            const c8* pos = parseChuck(iStart, iEnd);
            if(0 == pos) {
                return iStart;
            }
            iStart = pos;
            mReadChunk = false;

            if(0 == mChunkSize) {
                mReadHead = true;
                mPageFull = true;
                iStart = utility::AppGoNextLine(iStart, iEnd);
                continue;
            }
        }

        if(mPageSize > 0) {
            if(mReadedSize < mPageSize) {
                u32 need = mPageSize - mReadedSize;
                u32 buffersize = u32(iEnd - iStart);
                u32 writesize = core::min_<u32>(buffersize, need);

                writePage(iStart, writesize);
                mReadedSize += writesize;
                iStart += writesize;
            }

            if(mReadedSize == mPageSize) {
                iStart = utility::AppGoNextLine(iStart, iEnd);
                if(isByChunk()) {
                    mReadChunk = true;
                } else {
                    mReadHead = true;
                    mPageFull = true;
                }
            }
        } else {
            if(!isKeepAlive()) {
                u32 buffersize = u32(iEnd - iStart);
                writePage(iStart, buffersize);
                iStart += buffersize;
                mReadedSize += buffersize;
                break;
            } else {//with none content.
                iStart = iEnd;
                mReadHead = true;
                mPageFull = true;
            }
        }
    }//while

    return iStart;
}


void CNetHttpResponse::writeHTTP(const c8* iBuffer, u32 size) {
    if(mHTTP.getSize() + size < APP_MAXSIZE_HTTP) {
        mHTTP.addBuffer(iBuffer, size);
    }
}


void CNetHttpResponse::writePage(const c8* iBuffer, u32 size) {
    if(mWebPage.getSize() + size < APP_MAXSIZE_PAGE) {
        mWebPage.addBuffer(iBuffer, size);
    }
}


const c8* CNetHttpResponse::goValue(const c8* iStart, const c8* const iEnd) {
    iStart = 1 + utility::AppGoNextFlag(iStart, iEnd, ':');
    return utility::AppGoFirstWord(iStart, iEnd, false);
}


const c8* CNetHttpResponse::parseChuck(const c8* iStart, const c8* const iEnd) {
    if(mChunked) {
        if(iEnd - iStart <= 2) return 0;

        const c8* pos = utility::AppGoNextLine(iStart, iEnd) - 1;

        if(*pos != '\n') return 0;

        mChunkSize = core::strtoul16(iStart, &iStart);
        mPageSize += mChunkSize;
        iStart = utility::AppGoNextLine(iStart, iEnd);

        //mChunked = (0 != mPageSize);
    }
    return iStart;
}


const c8* CNetHttpResponse::parseHeadInner(const c8* iStart, const c8* const iEnd) {
    clear();

    const c8* npos;
    const c8* linetail;
    for(; iStart < iEnd; iStart = linetail + 1) {
        linetail = utility::AppGoNextLine(iStart, iEnd) - 1;
        switch(*iStart) {
        case 'H':
            if(*(iStart + 1) == 'T') {//HTTP/1.1 200
                npos = 1 + utility::AppGoNextFlag(iStart, linetail, '/');
                iStart = utility::AppGoNextFlag(npos, linetail, ' ');
                mHttpVersion = core::stringc(npos, u32(iStart - npos));

                iStart = utility::AppGoFirstWord(iStart, linetail, false);
                mStatusCode = (EHttpStatus) core::strtoul10(iStart, &iStart);
            }
            break;

        case 'c':
        case 'C':
            if(utility::AppCharEquals(*(iStart + 1), 'o')) {
                if(utility::AppCharEquals(*(iStart + 3), 'n')) {//Connection: close
                    iStart = goValue(iStart, linetail);
                    mKeepAlive = utility::AppCharEquals(*(iStart), 'k');
                } else if(utility::AppCharEquals(*(iStart + 8), 't')) {//Content-Type
                    npos = goValue(iStart, linetail);
                    iStart = utility::AppGoNextFlag(npos, linetail, '\r');
                    core::stringc node(npos, u32(iStart - npos));
                    mHead.setValue(EHHID_CONTENT_TYPE, node);
                } else if(utility::AppCharEquals(*(iStart + 8), 'm')) {//Content-MD5: base64
                    iStart = goValue(iStart, linetail);
                } else if(utility::AppCharEquals(*(iStart + 8), 'r')) {//Content-Range: 
                    iStart = goValue(iStart, linetail);
                } else if(utility::AppCharEquals(*(iStart + 8), 'e')) {//Content-Encoding: 
                    iStart = goValue(iStart, linetail);
                } else if(utility::AppCharEquals(*(iStart + 9), 'e')) {//Content-Length:
                    if(!mChunked) {//有了Transfer-Encoding，则不能有Content-Length
                        iStart = goValue(iStart, linetail);
                        mPageSize = core::strtol10(iStart, &iStart);
                    }
                } else if(utility::AppCharEquals(*(iStart + 9), 'o')) {//Content-Location:
                    iStart = goValue(iStart, linetail);
                }
            } else if(utility::AppCharEquals(*(iStart + 1), 'a')) {//Cache-Control: no-store
                iStart = goValue(iStart, linetail);
            }
            break;


        case 'l':
        case 'L':
            if(utility::AppCharEquals(*(iStart + 1), 'o')) {//Location:
                npos = goValue(iStart, linetail);
                iStart = utility::AppGoNextFlag(npos, linetail, '\r');
                core::stringc node(npos, u32(iStart - npos));
                mHead.setValue(EHHID_LOCATION, node);
            } else if(utility::AppCharEquals(*(iStart + 1), 'a')) {//Last-Modified:
                iStart = goValue(iStart, linetail);
            }
            break;

        case 't':
        case 'T':
            if(utility::AppCharEquals(*(iStart + 3), 'n')) {//Transfer-Encoding: chunked
                iStart = goValue(iStart, linetail);
                mChunked = utility::AppCharEquals(*(iStart), 'c');
            } else if(utility::AppCharEquals(*(iStart + 3), 'i')) {//Trailer: Max-Forwards
                iStart = goValue(iStart, linetail);
            }
            break;

        case 's':
        case 'S':
            if(utility::AppCharEquals(*(iStart + 1), 'e')) {
                if(utility::AppCharEquals(*(iStart + 2), 'r')) {//Server
                    iStart = goValue(iStart, linetail);
                    npos = iStart;
                    iStart = utility::AppGoNextFlag(iStart, linetail, '\r');
                    core::stringc val(npos, u32(iStart - npos));
                    mHead.setValue(EHRID_SERVER, val);
                } else if(utility::AppCharEquals(*(iStart + 2), 't')) {//Set-Cookie: path=/
                    iStart = goValue(iStart, linetail);
                }
            }
            break;

        case 'x':
        case 'X':
            if(*(iStart + 1) == '-') {
                if(utility::AppCharEquals(*(iStart + 2), 'p')) {//X-Powered-By
                    iStart = goValue(iStart, linetail);
                    npos = iStart;
                    iStart = utility::AppGoNextFlag(iStart, linetail, '\r');
                    u32 xsize = u32(iStart - npos);
                    const core::stringc* oldval = mHead.getValue(EHRID_X_POWERED_BY);
                    if(oldval) {
                        if(xsize + oldval->size() < APP_MAXSIZE_X_POWERED) {
                            core::stringc val(npos, xsize);
                            val.append(',');
                            val += *oldval;
                            mHead.setValue(EHRID_X_POWERED_BY, val);
                        }
                    } else {
                        core::stringc val(npos, xsize);
                        mHead.setValue(EHRID_X_POWERED_BY, val);
                    }
                } else if(utility::AppCharEquals(*(iStart + 2), 'c')) {//X-Cache: MISS from squid27
                    iStart = goValue(iStart, linetail);
                }
            }
            break;


        case 'W': //Warning:
                  //WWW-Authenticate:
        case 'R': //Retry-After:
        case 'P': //Pragma: no-cache
                  //Proxy-Authenticate
        case 'V': //Vary: Accept-Encoding
                  //Via:
        case 'D': //Date: Mon, 18 Dec 2017 21:13:14 GMT
        case 'E': //Expires: Thu, 19 Nov 1981 08:52:00 GMT
                  //ETag:
        default: break;
        }//switch
    }//for

    return iStart;
}


const c8* CNetHttpResponse::parseHead(const c8* const iStart, const c8* const iEnd) {
    //HTTP/1.1 200
    if(iEnd - iStart < 8) return 0;

    if('H' != *(iStart) || 'T' != *(iStart + 1)
        || 'T' != *(iStart + 2) || 'P' != *(iStart + 3)) {
        return 0;
    }

    const c8* const end = iEnd - 3;
    const c8* pos = iStart + 4;

    //http head end with "\r\n\r\n", or "\n\n"
    for(; pos <= end; ++pos) {
        if(*pos == '\n') {//line end
            if(*(pos + 1) == '\r' && *(pos + 2) == '\n') {//head end = double line end
                pos += 3;
                parseHeadInner(iStart, pos);
                return pos;
            }
        }
        if(*(pos + 1) == '\n') {//\n\n
            if(*(pos + 2) == '\n') {
                pos += 3;
                parseHeadInner(iStart, pos);
                return pos;
            }
        }
    }//for

    return 0;
}


void CNetHttpResponse::clear() {
    mPageFull = false;
    mChunked = false;
    mKeepAlive = true;
    mReadHead = true;
    mStatusCode = EHS_INVALID_CODE;
    mReadedSize = 0;
    mPageSize = 0;
    mChunkSize = 0;
    mHead.clear();
    mHTTP.shrink(APP_DEFAULT_SIZE_HTTP);
    mWebPage.shrink(APP_DEFAULT_SIZE_PAGE);
}

} //namespace net
} //namespace irr