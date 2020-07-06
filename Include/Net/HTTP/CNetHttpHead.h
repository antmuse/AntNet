#ifndef APP_CNETHTTPHEAD_H
#define APP_CNETHTTPHEAD_H

#include "CString.h"
#include "AppMap.h"


namespace app {
namespace net {
class CNetPacket;


class CNetHttpHead {
public:

    CNetHttpHead();

    ~CNetHttpHead();

    const core::CString* getValue(const core::CString& id)const {
        THttpHeadNode* it = mMapHead.find(id);
        return (it ? &it->getValue() : nullptr);
    }


    void setValue(const core::CString& id, const core::CString& it) {
        mMapHead.set(id, it);
    }


    void removeHead(const core::CString& id) {
        mMapHead.remove(id);
    }


    void getBuffer(CNetPacket& out)const;


    void clear() {
        mMapHead.clear();
    }


private:
    typedef core::TMap<core::CString, core::CString>::Node THttpHeadNode;
    typedef core::TMap<core::CString, core::CString>::Iterator THttpHeadIterator;
    core::TMap<core::CString, core::CString> mMapHead;
};


} //namespace net
} //namespace app

#endif //APP_CNETHTTPHEAD_H
