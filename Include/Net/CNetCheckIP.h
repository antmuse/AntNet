#ifndef APP_CNETCHECKIP_H
#define APP_CNETCHECKIP_H

#include "HNetConfig.h"

#define APP_MAX_IP_CHECK_NODE 30
#define APP_IP_SPLIT_FLAG ','
#define APP_IP_CHECK_INVALID_RESULT 0xFFFFFFFFU

namespace irr {
namespace net {


class CNetCheckIP {
public:
    struct SNetIPNode {
        u32 m_min;
        u32 m_max;

        SNetIPNode() : m_min(0), m_max(0) {
        }

        SNetIPNode(const SNetIPNode& it) : m_min(it.m_min), m_max(it.m_max) {
        }

        SNetIPNode& operator=(const SNetIPNode& it) {
            m_min = it.m_min;
            m_max = it.m_max;
            return *this;
        }

        void swap(SNetIPNode& it) {
            SNetIPNode tmp = it;
            it = *this;
            *this = tmp;
        }

        inline bool needMerge(const SNetIPNode& it)const {
            return (m_max + 1 == it.m_min || it.m_max + 1 == m_min);
        }

        void mergeWith(const SNetIPNode& it) {
            if(it.m_min < m_min) {
                m_min = it.m_min;
            }
            if(it.m_max > m_max) {
                m_max = it.m_max;
            }
        }

        //@param ip little endian as OS
        //@return 0 if equal or included
        //@return 1 if this is bigger than ip.
        //@return -1 if this is smaller than ip.
        inline int compareWith(const u32 ip)const {
            if(ip >= m_min && ip <= m_max) {
                return 0;
            }
            return (ip < m_min ? 1 : -1);
        }

        //@return 0 if overlapped
        //@return 1 if this is bigger than it.
        //@return -1 if this is smaller than it.
        inline int compareWith(const SNetIPNode& it)const {
            if(m_min > it.m_max) {
                return 1;
            }
            if(m_max < it.m_min) {
                return -1;
            }
            return 0;
        }

        void checkOrder() {
            if(m_min > m_max) {
                u32 tmp = m_min;
                m_min = m_max;
                m_max = tmp;
            }
        }
    };

    CNetCheckIP();

    ~CNetCheckIP();

    CNetCheckIP(const CNetCheckIP& it);

    CNetCheckIP& operator=(const CNetCheckIP& it);

    bool isValidIP(const c8* str)const {
        return (str ? isValidLittleEndianIP(convertToIP(str)) : false);
    }

    //@param ip IP in little endian
    bool isValidLittleEndianIP(u32 ip)const;

    //@param ip IP in big endian
    bool isValidBigEndianIP(u32 ip)const {
        return isValidLittleEndianIP(APP_SWAP32(ip));
    }

    //@return size of nodes.
    u32 parseConfig(const c8* cfg, u32 size = 0);

    void remove(u32 idx);

    void clear();

    u32 getSize()const {
        return m_size;
    }

    //@return IP in little endian.
    static u32 convertToIP(const c8* str);

private:
    SNetIPNode m_data[APP_MAX_IP_CHECK_NODE];
    u32 m_size;	//count of SNetIPNode

    bool addNode(const c8* iStart, const c8* iEnd);

    u32 insert(SNetIPNode& it);

    u32 erodeFront(u32 idx, SNetIPNode& it);

    u32 erodeBack(u32 idx, SNetIPNode& it);
};

};//namespace net
};//namespace irr


#endif //APP_CNETCHECKIP_H

