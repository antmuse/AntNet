#ifndef APP_IAPPTIMER_H
#define APP_IAPPTIMER_H

#include "HConfig.h"
#include "irrTypes.h"

namespace irr {

/// Interface for date and time
class IAppTimer {
public:
    enum EWeekday {
        EWD_SUNDAY = 0,
        EWD_MONDAY,
        EWD_TUESDAY,
        EWD_WEDNESDAY,
        EWD_THURSDAY,
        EWD_FRIDAY,
        EWD_SATURDAY
    };

    struct SDate {
        // Hour of the day, from 0 to 23
        u32 mHour;
        // Minute of the hour, from 0 to 59
        u32 mMinute;
        // Second of the minute, due to extra seconds from 0 to 61
        u32 mSecond;
        // Year of the gregorian calender
        u32 mYear;
        // Month of the year, from 1 to 12
        u32 mMonth;
        // Day of the month, from 1 to 31
        u32 mDay;
        // Weekday for the current day
        EWeekday mWeekday;
        // Day of the year, from 1 to 366
        u32 mYearday;
        // Whether daylight saving is on
        bool mIsDST;
    };

    static SDate getDate(s64 time);

    static SDate getDate();

    static void getDate(SDate& date);

    static void getDate(s64 time, SDate& date);

    static bool isLeapYear(u32 iYear);

    /**
    * @return time in seconds
    */
    static s64 cutDays(s64 timestamp);

    /**
    * @return time zone in seconds
    */
    static s64 getTimeZone();

    /**
    * @return time in seconds
    */
    static s64 getTimestamp();

    /**
    * @return time in milliseconds
    */
    static s64 getTime();

    /**
    * @return time in microseconds
    */
    static s64 getRealTime();

    /**
    * @return time in milliseconds
    */
    static s64 getRelativeTime();

    static u32 getMonthMaxDay(u32 iYear, u32 iMonth);

    /**
    *@brief Get current time as string.
    *@param iTime timestamp in seconds.
    *@param format format str, eg:
    *   "%a, %d %b %Y %H:%M:%S GMT", //Sat, 11 Mar 2017 21:49:51 GMT
    *   "%Y-%m-%d %H:%M:%S"          //2017-07-09 21:49:51

    *@param cache The cache to write into.
    *@param cacheSize The max cache size, should not be less than 20.
    *@return len of output str.
    */
    static u64 getTimeAsString(s64 iTime, c8* cache, u32 cacheSize, c8* format = "%Y-%m-%d %H:%M:%S");
    static u64 getTimeAsString(s64 iTime, wchar_t* cache, u32 cacheSize, wchar_t* format = L"%Y-%m-%d %H:%M:%S");

    static u64 getTimeAsString(c8* cache, u32 cacheSize, c8* format = "%Y-%m-%d %H:%M:%S");
    static u64 getTimeAsString(wchar_t* cache, u32 cacheSize, wchar_t* format = L"%Y-%m-%d %H:%M:%S");
};

} // end namespace irr

#endif //APP_IAPPTIMER_H
