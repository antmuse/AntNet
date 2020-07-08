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

    CNetHttpHead(const CNetHttpHead& val) {
        *this = val;
    }

    ~CNetHttpHead();

    CNetHttpHead& operator=(const CNetHttpHead& val);

    /**
    * @return 查找成功返回不可修改或删除的value, 否则返回nullptr.
    */
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

    void show();


private:
    typedef core::TMap<core::CString, core::CString>::Node THttpHeadNode;
    typedef core::TMap<core::CString, core::CString>::Iterator THttpHeadIterator;
    typedef core::TMap<core::CString, core::CString>::ConstIterator THttpHeadConstIterator;
    core::TMap<core::CString, core::CString> mMapHead;
};


} //namespace net
} //namespace app

#endif //APP_CNETHTTPHEAD_H
