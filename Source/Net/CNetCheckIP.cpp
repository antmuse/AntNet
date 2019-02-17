#include "CNetCheckIP.h"
#include "IUtility.h"
//#include "fast_atof.h"

#if !defined(NDEBUG)
#if defined(APP_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <WS2tcpip.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif  //APP_PLATFORM_WINDOWS
#endif


namespace irr {
namespace net {

CNetCheckIP::CNetCheckIP() : m_size(0) {
}


CNetCheckIP::~CNetCheckIP() {
}


bool CNetCheckIP::isValidLittleEndianIP(u32 ip)const {
    if(0 == m_size) {
        return true;
    }
    s32 left = 0;
    s32 right = (s32) (m_size - 1);
    s32 mid = 0;
    s32 result;
    while(left <= right) {
        mid = left + ((right - left) >> 1);
        result = m_data[mid].compareWith(ip);
        if(0 == result) {
            return true;
        }
        if(result < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return false;
}


bool CNetCheckIP::addNode(const c8* iStart, const c8* iEnd) {
    APP_ASSERT(iStart && iEnd);
    if(m_size >= APP_MAX_IP_CHECK_NODE) {//full
        return false;
    }

    SNetIPNode node;
    const c8* curr = utility::AppGoNextFlag(iStart, iEnd, '/');
    if(curr > iStart && curr < iEnd) {//found eg: "192.168.1.1/24"
        const u32 bitcount = ::atoi(curr + sizeof(char));
        node.m_min = convertToIP(iStart);
        if(0 == bitcount || bitcount >= 32) {
            node.m_max = node.m_min;
            insert(node);
            return true;
        }
        const u32 mask = (0xFFFFFFFF >> bitcount);	//32 bits for IPV4
        node.m_min &= (~mask);
        node.m_max = node.m_min + (mask - 1);
        ++node.m_min;
        /*printf("CNetCheckIP::addNode>> little endian, add span = %u\n",
        (node.m_max)-(node.m_min)+1);
        printf("CNetCheckIP::addNode>> big endian, add span = %u\n",
        APP_SWAP_32(node.m_max)-APP_SWAP_32(node.m_min)+1);*/
        insert(node);
        return true;
    }
    curr = utility::AppGoNextFlag(iStart, iEnd, '-');
    if(curr > iStart && curr < iEnd) {//found eg: "192.168.1.1-192.168.1.123"
        node.m_min = convertToIP(iStart);
        node.m_max = convertToIP(curr + sizeof(char));
        node.checkOrder();
        insert(node);
        return true;
    }

    //normal format eg: "127.0.0.1"
    node.m_min = convertToIP(iStart);
    node.m_max = node.m_min;
    insert(node);
    return true;
}


u32 CNetCheckIP::convertToIP(const c8* str) {
    u32 ret = 0; //little endian
#if (0)
    if(str) {
        const c8* iEnd = str + ::strlen(str);
        ret = (::atoi(str) & 0x000000FF) << 24;
        str = utility::AppStringGoNextFlag(str, iEnd, '.');
        if(str >= iEnd) {
            return ret;
        }
        ret |= (::atoi(++str) & 0x000000FF) << 16;
        str = utility::AppStringGoNextFlag(str, iEnd, '.');
        if(str >= iEnd) {
            return ret;
        }
        ret |= (::atoi(++str) & 0x000000FF) << 8;
        str = utility::AppStringGoNextFlag(str, iEnd, '.');
        if(str >= iEnd) {
            return ret;
        }
        ret |= (::atoi(++str) & 0x000000FF);
    }
#endif
    if(str) {
        ret = (core::strtoul10(str, &str) & 0x000000FF) << 24;
        ret |= (core::strtoul10('.' == *str ? ++str : str, &str) & 0x000000FF) << 16;
        ret |= (core::strtoul10('.' == *str ? ++str : str, &str) & 0x000000FF) << 8;
        ret |= (core::strtoul10('.' == *str ? ++str : str, &str) & 0x000000FF);
    }
    return ret;
}


u32 CNetCheckIP::insert(SNetIPNode& it) {
    if(0 == m_size) {
        m_data[m_size++] = it;
        return 0;
    } else if(m_size >= APP_MAX_IP_CHECK_NODE) {//full
        return APP_IP_CHECK_INVALID_RESULT;
    }

    s32 left = 0;
    s32 right = (s32) (m_size - 1);
    s32 mid = 0;
    s32 result = 0;
    while(left <= right) {
        mid = left + ((right - left) >> 1);
        result = m_data[mid].compareWith(it);
        if(0 == result) {//found success
            break;
        }
        if(result < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }//while

    u32 gap1 = erodeFront(mid, it);
    u32 gap2 = erodeBack(mid, it);
    if(gap1 > 0 || gap2 > 0) {
        m_data[mid - gap1] = it;
        if(gap1 + gap2 > 1) {
            if(m_size - mid - gap2 > 0) {
                ::memmove(m_data + mid - gap1 + 1, m_data + mid + gap2, (m_size - mid - gap2) * sizeof(SNetIPNode));
            }
            m_size -= (gap1 + gap2 - 1);
        }
        return mid - gap1;
    }

    //not eroded
    if(result < 0) {//insert on right side of current position
        ++mid;
    }
    if(mid < (s32) m_size) {
        ::memmove(m_data + mid + 1, m_data + mid, (m_size - mid) * sizeof(SNetIPNode));
    }
    m_data[mid] = it;
    ++m_size;
    return mid;
}

u32 CNetCheckIP::erodeFront(u32 idx, SNetIPNode& it) {
    u32 gap = 0;
    if(idx + 1 > m_size) {
        return gap;
    }
    for(s32 i = (s32) idx - 1; i >= 0; --i) {
        if(it.needMerge(m_data[i]) || 0 == it.compareWith(m_data[i])) {
            it.mergeWith(m_data[i]);
            ++gap;
        } else {
            break;
        }
    }
    return gap;
}


u32 CNetCheckIP::erodeBack(u32 idx, SNetIPNode& it) {
    u32 gap = 0;
    if(idx + 1 > m_size) {
        return gap;
    }
    for(u32 i = idx; i < m_size; ++i) {
        if(it.needMerge(m_data[i]) || 0 == it.compareWith(m_data[i])) {
            it.mergeWith(m_data[i]);
            ++gap;
        } else {
            break;
        }
    }//for
    return gap;
}


u32 CNetCheckIP::parseConfig(const c8* cfg, u32 cfgsize/*=0*/) {
    if(!cfg || '\0' == *cfg) {
        return m_size;
    }
    if(0 == cfgsize) {
        cfgsize = (u32)::strlen(cfg);
    }
    const c8* iStart = cfg;
    const c8* iEnd = iStart + cfgsize;
    const c8* curr = utility::AppGoNextFlag(iStart, iEnd, APP_IP_SPLIT_FLAG);
    bool added = true;
    while(added && curr > iStart && iStart < iEnd) {
        added = addNode(iStart, curr);
        iStart = curr + sizeof(char);
        iStart = utility::AppGoFirstWord(iStart, iEnd, false);
        curr = utility::AppGoNextFlag(iStart, iEnd, ',');
    }
    return m_size;
}


void CNetCheckIP::remove(u32 idx) {
    if(idx < m_size) {
        if(idx < (m_size - 1)) {
            ::memmove(m_data + idx, m_data + (1 + idx), (m_size - 1 - idx) * sizeof(SNetIPNode));
        }
        --m_size;
    }
}


void CNetCheckIP::clear() {
    m_size = 0;
}


#if !defined(NDEBUG)
void AppTestNetIP() {
    CNetCheckIP cker;
    u32 tmp1 = 0;
    ::inet_pton(AF_INET, "127.2.3.4", &tmp1);
    u32 tmp2 = APP_SWAP32(CNetCheckIP::convertToIP("127.2.3.4"));
    printf("tmp1 %c= tmp2\n", tmp1 == tmp2 ? '=' : '!');
    tmp1 = APP_SWAP32(CNetCheckIP::convertToIP("127.a"));
    tmp1 = APP_SWAP32(CNetCheckIP::convertToIP(""));
    tmp1 = APP_SWAP32(CNetCheckIP::convertToIP("-@+.fafd..asd"));

    cker.parseConfig("");
    cker.parseConfig(NULL);
    cker.parseConfig("127.2.3.4,");
    cker.parseConfig("127.2.3.4-127.2.3.42");
    cker.parseConfig("127.2.3.43-127.2.3.48");
    cker.parseConfig("128.2.3.43/24");
    cker.parseConfig("129.2.3.44-129.2.3.180");
    cker.parseConfig("129.2.3.15-129.2.3.28,129.2.3.1-129.2.3.8");
    cker.parseConfig("129.2.3.10-129.2.3.12");
    printf("127.2.3.111 = %s\n", cker.isValidIP("127.2.3.111") ? "true" : "false");
    printf("checker's size = %u\n", cker.getSize());
    cker.parseConfig("127.2.3.4-127.2.3.111");
    cker.parseConfig("129.2.3.1-129.2.3.211");
    printf("checker's size = %u\n", cker.getSize());
    cker.parseConfig("126.2.3.1/24,125.1.2.3");
    printf("127.2.3.3 = %s\n", cker.isValidIP("127.2.3.3") ? "true" : "false");
    printf("127.2.3.41 = %s\n", cker.isValidIP("127.2.3.41") ? "true" : "false");
    printf("127.2.3.42 = %s\n", cker.isValidIP("127.2.3.42") ? "true" : "false");
    printf("127.2.3.4 = %s\n", cker.isValidIP("127.2.3.4") ? "true" : "false");
    printf("127.2.3.49 = %s\n", cker.isValidIP("127.2.3.49") ? "true" : "false");
    printf("127.2.3.48 = %s\n", cker.isValidIP("127.2.3.48") ? "true" : "false");
    printf("128.2.3.43 = %s\n", cker.isValidIP("128.2.3.43") ? "true" : "false");
    printf("129.2.3.4 = %s\n", cker.isValidIP("129.2.3.4") ? "true" : "false");
    printf("129.2.3.3 = %s\n", cker.isValidIP("129.2.3.3") ? "true" : "false");
    printf("127.2.3.111 = %s\n", cker.isValidIP("127.2.3.111") ? "true" : "false");
    printf("checker's size = %u\n", cker.getSize());
}
#endif

};//namespace net
};//namespace irr


#if !defined(NDEBUG)
irr::s32 test_main(irr::s32 argc, irr::c8** argv) {
    irr::net::AppTestNetIP();
    return 0;
}
#endif
