#ifndef APP_CNETHTTPRESPONSE_H
#define APP_CNETHTTPRESPONSE_H

#include "irrMap.h"
#include "irrString.h"
#include "HNetHttpStatus.h"
#include "CNetHttpHead.h"
#include "CNetPacket.h"

namespace irr {
namespace net {


class CNetHttpResponse {
public:
    //ContentType: text/html
    enum EContentType {
        ECT_TEXT,
        ECT_VIDEO,
        ECT_AUDIO,
        ECT_IMAGE,
        ECT_APPLICATION,
    };
    CNetHttpResponse();

    ~CNetHttpResponse();

    CNetHttpHead& getHead() {
        return mHead;
    }

    const CNetHttpHead& getHead() const {
        return mHead;
    }

    u32 getPageSize()const {
        return mWebPage.getSize();
    }

    const CNetPacket& getHTTP() const {
        return mHTTP;
    }

    const CNetPacket& getPage() const {
        return mWebPage;
    }

    void writeHTTP(const c8* iBuffer, u32 size);

    void writePage(const c8* iBuffer, u32 size);

    u32 getStatusCode() const {
        return mStatusCode;
    }

    const c8* import(const c8* iBuffer, u32 size);

    bool isFull()const {
        return mPageFull;
    }

    u32 getContentSize()const {
        return mPageSize;
    }

    u32 getChunkSize()const {
        return mChunkSize;
    }

    void clear();

    bool isByChunk()const {
        return mChunked;
    }

    bool isKeepAlive()const {
        return mKeepAlive;
    }


private:
    /**
    *@brief
    *eg: "Connection: close" or "Connection:close"
    */
    const c8* goValue(const c8* iStart, const c8* const iEnd);

    const c8* parseHead(const c8* const iBuffer, const c8* const iEnd);

    const c8* parseChuck(const c8* const iStart, const c8* const iEnd);

    const c8* parseHeadInner(const c8* const iStart, const c8* const iEnd);
    //void createFile(u32 size);

    EHttpStatus mStatusCode;
    bool mPageFull;
    bool mChunked;              ///<Transfer-Encoding: chunked
    bool mKeepAlive;            ///<Connection: close/keep-alive
    bool mReadHead;             ///<read head
    bool mReadChunk;            ///<read chunk line
    u32 mReadedSize;
    u32 mChunkSize;
    u32 mPageSize;
    core::stringc mHttpVersion;
    CNetHttpHead mHead;
    CNetPacket mWebPage;
    CNetPacket mHTTP;
};


} //namespace net
} //namespace irr

#endif //APP_CNETHTTPRESPONSE_H
