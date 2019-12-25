#include "IAppTimer.h"
#include <stdio.h>
#include <time.h>
#if defined(APP_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
#include <sys/time.h>
#endif

namespace irr {

s64 AppGetTimeZone() {
    long ret;
    //set time_zone
    time_t tms = 0;
    tm gm;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&gm, &tms);
    _get_timezone(&ret);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    ret = tz.tz_dsttime * 60;
#endif
    return ret;
}


void AppDateConvert(const struct tm& timeinfo, IAppTimer::SDate& date) {
    date.mHour = ( u32) timeinfo.tm_hour;
    date.mMinute = ( u32) timeinfo.tm_min;
    date.mSecond = ( u32) timeinfo.tm_sec;
    date.mDay = ( u32) timeinfo.tm_mday;
    date.mMonth = ( u32) timeinfo.tm_mon + 1;
    date.mYear = ( u32) timeinfo.tm_year + 1900;
    date.mWeekday = (IAppTimer::EWeekday)timeinfo.tm_wday;
    date.mYearday = ( u32) timeinfo.tm_yday + 1;
    date.mIsDST = timeinfo.tm_isdst != 0;
}


void IAppTimer::getDate(IAppTimer::SDate& date) {
    time_t rawtime = time(nullptr);
    struct tm timeinfo;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&timeinfo, &rawtime);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    localtime_r(&rawtime, &timeinfo);
#endif
    AppDateConvert(timeinfo, date);
}


void IAppTimer::getDate(s64 rawtime, IAppTimer::SDate& date) {
    struct tm timeinfo;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&timeinfo, &rawtime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    localtime_r(( time_t*) & rawtime, &timeinfo);
#endif
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


s64 IAppTimer::getRelativeTime() {
#if defined(APP_PLATFORM_WINDOWS)
#if defined(APP_OS_64BIT)
    return ::GetTickCount64();
#else
    return ::GetTickCount();
#endif
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);
#endif
}

s64 IAppTimer::getTimestamp() {
    return time(nullptr);
}

s64 IAppTimer::getTimeZone() {
    static s64 ret = AppGetTimeZone();
    return ret;
}

s64 IAppTimer::cutDays(s64 timestamp) {
    const s64 daysec = 24 * 60 * 60;
    return (timestamp - getTimeZone()) / daysec * daysec + getTimeZone();
}


#if defined(APP_PLATFORM_WINDOWS)
static BOOL HighPerformanceTimerSupport = FALSE;
static BOOL MultiCore = FALSE;
LARGE_INTEGER AppGetSysFrequency() {
    LARGE_INTEGER ret;
    HighPerformanceTimerSupport = QueryPerformanceFrequency(&ret);
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    MultiCore = (sysinfo.dwNumberOfProcessors > 1);
    return ret;
}
s64 AppGetHardwareTime() {
    static LARGE_INTEGER HighPerformanceFreq = AppGetSysFrequency();
    if(HighPerformanceTimerSupport) {
        // Avoid potential timing inaccuracies across multiple cores by
        // temporarily setting the affinity of this process to one core.
        DWORD_PTR affinityMask = 0;
        if(MultiCore) {
            affinityMask = SetThreadAffinityMask(GetCurrentThread(), 1);
        }
        LARGE_INTEGER nTime;
        BOOL queriedOK = QueryPerformanceCounter(&nTime);
        if(MultiCore) {// Restore the true affinity.
            SetThreadAffinityMask(GetCurrentThread(), affinityMask);
        }
        if(queriedOK) {
            return ((nTime.QuadPart) * 1000LL / HighPerformanceFreq.QuadPart);
        }
    }
    return time(nullptr);
}
#endif


s64 IAppTimer::getTime() {
#if defined(APP_PLATFORM_WINDOWS)
    struct tm tms;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tms.tm_year = wtm.wYear - 1900;
    tms.tm_mon = wtm.wMonth - 1;
    tms.tm_mday = wtm.wDay;
    tms.tm_hour = wtm.wHour;
    tms.tm_min = wtm.wMinute;
    tms.tm_sec = wtm.wSecond;
    tms.tm_isdst = -1;
    return mktime(&tms) * 1000LL + wtm.wMilliseconds;
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    timeval tv;
    gettimeofday(&tv, 0);
    return ((tv.tv_sec * 1000LL) + tv.tv_usec / 1000LL);
#endif
}

s64 IAppTimer::getRealTime() {
#if defined(APP_PLATFORM_WINDOWS)
    static const u64 FILETIME_to_timval_skew = 116444736000000000ULL;
    FILETIME tfile;
    //这个函数获取到的是从1601年1月1日到目前经过的纳秒
    ::GetSystemTimeAsFileTime(&tfile);

    ULARGE_INTEGER _100ns;
    _100ns.LowPart = tfile.dwLowDateTime;
    _100ns.HighPart = tfile.dwHighDateTime;
    _100ns.QuadPart -= FILETIME_to_timval_skew;

    //timeval timenow;
    //// Convert 100ns units to seconds;
    //timenow.tv_sec = ( long) (_100ns.QuadPart / (10000 * 1000));
    //// Convert remainder to microseconds;
    //timenow.tv_usec = ( long) ((_100ns.QuadPart % (10000 * 1000)) / 10);
    //return (timenow.tv_sec * 1000000LL + timenow.tv_usec);

    return (_100ns.QuadPart / 10);

#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
#endif
}

u64 IAppTimer::getTimeAsString(s64 iTime, c8* cache, u32 max, c8* format) {
    struct tm timeinfo;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    localtime_r(( time_t*) & iTime, &timeinfo);
#endif
    return strftime(cache, max, format, &timeinfo);
}

u64 IAppTimer::getTimeAsString(c8* cache, u32 max, c8* format) {
    s64 iTime = ::time(nullptr);
    struct tm timeinfo;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    localtime_r(( time_t*) & iTime, &timeinfo);
#endif
    return strftime(cache, max, format, &timeinfo);
}


u64 IAppTimer::getTimeAsString(s64 iTime, wchar_t* cache, u32 max, wchar_t* format) {
    struct tm timeinfo;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    localtime_r(( time_t*) & iTime, &timeinfo);
#endif
    return wcsftime(cache, max, format, &timeinfo);
}

u64 IAppTimer::getTimeAsString(wchar_t* cache, u32 max, wchar_t* format) {
    s64 iTime = ::time(nullptr);
    struct tm timeinfo;
#if defined(APP_PLATFORM_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(APP_PLATFORM_LINUX) || defined(APP_PLATFORM_ANDROID)
    localtime_r(( time_t*) & iTime, &timeinfo);
#endif
    return wcsftime(cache, max, format, &timeinfo);
}


bool IAppTimer::isLeapYear(u32 iYear) {
    return ((0 == iYear % 4 && 0 != iYear % 100) || 0 == iYear % 400);
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
