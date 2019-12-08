#include "IAppTimer.h"
#include <stdio.h>
#include <time.h>
#if defined(APP_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(APP_PLATFORM_LINUX)
#include <sys/time.h>
#endif

namespace irr {

void AppDateConvert(const struct tm* timeinfo, IAppTimer::SDate& date) {
    if(timeinfo) {	// set useful values if succeeded
        date.mHour = (u32) timeinfo->tm_hour;
        date.mMinute = (u32) timeinfo->tm_min;
        date.mSecond = (u32) timeinfo->tm_sec;
        date.mDay = (u32) timeinfo->tm_mday;
        date.mMonth = (u32) timeinfo->tm_mon + 1;
        date.mYear = (u32) timeinfo->tm_year + 1900;
        date.mWeekday = (IAppTimer::EWeekday)timeinfo->tm_wday;
        date.mYearday = (u32) timeinfo->tm_yday + 1;
        date.mIsDST = timeinfo->tm_isdst != 0;
    }
}


void IAppTimer::getDate(IAppTimer::SDate& date) {
    time_t rawtime;
    ::time(&rawtime);
    struct tm* timeinfo = ::localtime(&rawtime);
    AppDateConvert(timeinfo, date);
}


void IAppTimer::getDate(s64 userTime, IAppTimer::SDate& date) {
    time_t t = userTime;
    struct tm* timeinfo = ::gmtime(&t);
    AppDateConvert(timeinfo, date);
}


IAppTimer::SDate IAppTimer::getDate() {
    IAppTimer::SDate date = {0};
    getDate(date);
    return date;
}


IAppTimer::SDate IAppTimer::getDate(s64 time) {
    IAppTimer::SDate date = {0};
    getDate(time, date);
    return date;
}


s64 IAppTimer::getTime() {
#if defined(APP_PLATFORM_WINDOWS)
    /*
    static LARGE_INTEGER HighPerformanceFreq;
    static BOOL HighPerformanceTimerSupport = FALSE;
    static BOOL MultiCore = FALSE;

    if (HighPerformanceTimerSupport)	{
    // Avoid potential timing inaccuracies across multiple cores by
    // temporarily setting the affinity of this process to one core.
    DWORD_PTR affinityMask=0;
    if(MultiCore)
    affinityMask = SetThreadAffinityMask(GetCurrentThread(), 1);

    LARGE_INTEGER nTime;
    BOOL queriedOK = QueryPerformanceCounter(&nTime);

    // Restore the true affinity.
    if(MultiCore)
    (void)SetThreadAffinityMask(GetCurrentThread(), affinityMask);

    if(queriedOK)
    return u32((nTime.QuadPart) * 1000 / HighPerformanceFreq.QuadPart);

    }*/

#if defined(APP_OS_64BIT)
    return ::GetTickCount64();
#else
    return ::GetTickCount();
#endif
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    timeval tv;
    ::gettimeofday(&tv, 0);
    return (s64) ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
#endif
}


s32 IAppTimer::addYearMonthDay(const IAppTimer::SDate& dt, u32 max, c8* cache) {
    if(dt.mMonth < 10) {
        if(dt.mDay < 10) {
            return snprintf(cache, max, "%d_0%d_0%d", dt.mYear, dt.mMonth, dt.mDay);
        } else {
            return snprintf(cache, max, "%d_0%d_%d", dt.mYear, dt.mMonth, dt.mDay);
        }
    } else {
        if(dt.mDay < 10) {
            return snprintf(cache, max, "%d_%d_0%d", dt.mYear, dt.mMonth, dt.mDay);
        } else {
            return snprintf(cache, max, "%d_%d_%d", dt.mYear, dt.mMonth, dt.mDay);
        }
    }
}


s32 IAppTimer::addHourMinSecond(const IAppTimer::SDate& dt, u32 max, c8* cache) {
    if(dt.mHour < 10) {
        if(dt.mMinute < 10) {
            if(dt.mSecond < 10) {
                return snprintf(cache, max, "0%d:0%d:0%d", dt.mHour, dt.mMinute, dt.mSecond);
            } else {
                return snprintf(cache, max, "0%d:0%d:%d", dt.mHour, dt.mMinute, dt.mSecond);
            }
        } else {
            if(dt.mSecond < 10) {
                return snprintf(cache, max, "0%d:%d:0%d", dt.mHour, dt.mMinute, dt.mSecond);
            } else {
                return snprintf(cache, max, "0%d:%d:%d", dt.mHour, dt.mMinute, dt.mSecond);
            }
        }
    } else {
        if(dt.mMinute < 10) {
            if(dt.mSecond < 10) {
                return snprintf(cache, max, "%d:0%d:0%d", dt.mHour, dt.mMinute, dt.mSecond);
            } else {
                return snprintf(cache, max, "%d:0%d:%d", dt.mHour, dt.mMinute, dt.mSecond);
            }
        } else {
            if(dt.mSecond < 10) {
                return snprintf(cache, max, "%d:%d:0%d", dt.mHour, dt.mMinute, dt.mSecond);
            } else {
                return snprintf(cache, max, "%d:%d:%d", dt.mHour, dt.mMinute, dt.mSecond);
            }
        }
    }
}


void IAppTimer::getTimeAsString(u32 type, c8* cache, u32 max) {
    if(max < 20) {
        cache[0] = '\0';
        return;
    }
    IAppTimer::SDate dt = IAppTimer::getDate();
    s32 ret = 0;
    switch(type) {
    case ETST_YMD:
        ret = addYearMonthDay(dt, max, cache);
        break;
    case ETST_YMD_2:
        ret = addYearMonthDay(dt, max, cache);
        cache[4] = '-';
        cache[7] = '-';
        break;
    case ETST_HMS:
        ret = addHourMinSecond(dt, max, cache);
        break;
    case ETST_YMDHMS:
        ret = addYearMonthDay(dt, max, cache);
        cache[4] = '-';
        cache[7] = '-';
        cache[ret++] = ' ';
        ret += addHourMinSecond(dt, max - ret, cache + ret);
        break;
    case ETST_GMT:
    {   //TODO>>
        time_t rawTime;
        struct tm* timeInfo;
        ::time(&rawTime);
        timeInfo = ::gmtime(&rawTime);
        ::strftime(cache, max, "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
    }
    break;
    }//switch
}


void IAppTimer::getTimeAsString(const IAppTimer::SDate& dt, u32 type, c8* cache, u32 max) {
    if(max < 20) {
        cache[0] = '\0';
        return;
    }
    s32 ret = 0;

    switch(type) {
    case ETST_YMD:
        ret = addYearMonthDay(dt, max, cache);
        break;
    case ETST_HMS:
        ret = addHourMinSecond(dt, max, cache);
        break;
    case ETST_YMDHMS:
        ret = addYearMonthDay(dt, max, cache);
        cache[4] = '-';
        cache[7] = '-';
        cache[ret++] = ' ';
        ret += addHourMinSecond(dt, max - ret, cache + ret);
        break;
    }//switch
}


bool IAppTimer::isLeapYear(u32 iYear) {
    if((0 == iYear % 4 && 0 != iYear % 100) || 0 == iYear % 400) {
        return true;
    }
    return false;
}


u32 IAppTimer::getMonthMaxDay(u32 iYear, u32 iMonth) {
    switch(iMonth) {
    case 2:
        return IAppTimer::isLeapYear(iYear) ? 29 : 28;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    default:
        break;
    }
    return 31;
}


} // end namespace irr
