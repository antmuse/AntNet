#ifndef APP_IAPPTIMER_H
#define APP_IAPPTIMER_H

#include "HConfig.h"
#include "irrTypes.h"

namespace irr {

    /// Interface for date and time
    class IAppTimer {
    public:
        enum EWeekday {
            EWD_SUNDAY=0,
            EWD_MONDAY,
            EWD_TUESDAY,
            EWD_WEDNESDAY,
            EWD_THURSDAY,
            EWD_FRIDAY,
            EWD_SATURDAY
        };

        enum ETimeStringType{
            ETST_YMD = 0,            ///<format: year_month_day, eg: 2017_07_09
            ETST_YMD_2,             ///<format: year_month_day, eg: 2017-07-09
            ETST_HMS,                   ///<format: Hour_mintue_second, eg: 09:30:22
            ETST_YMDHMS          ///<format: year_month_day_Hour_mintue_second, eg: 2017-07-09 09:30:22
        };

        struct SDate {
            // Hour of the day, from 0 to 23
            u32 mHour;
            // Minute of the hour, from 0 to 59
            u32 mMinute;
            // Second of the minute, due to extra seconds from 0 to 61
            u32 mSecond;
            // Year of the gregorian calender
            s32 mYear;
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

        static const IAppTimer::SDate& getDate();

        static bool isLeapYear(u32 iYear);

        static u32 getTime();

        static u32 getMonthMaxDay(u32 iYear, u32 iMonth);

        static const c8* getTimeAsString(u32 type = ETST_YMD);

        static const c8* getTimeAsString(const IAppTimer::SDate& date, u32 type = ETST_YMD);

        static s32 addYearMonthDay(const IAppTimer::SDate& date, u32 max, c8* cache);

        static s32 addHourMinSecond(const IAppTimer::SDate& date, u32 max, c8* cache);
    };

} // end namespace irr

#endif //APP_IAPPTIMER_H
