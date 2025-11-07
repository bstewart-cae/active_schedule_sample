/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "rtc.h"
#define MS_IN_SECOND 1000
#define MS_IN_MINUTE          (MS_IN_SECOND * 60)
#define MS_IN_HOUR            (MS_IN_MINUTE * 60)
#define MS_IN_DAY             (MS_IN_HOUR * 24)
#define SECONDS_IN_DAY        (24 * 60 * 60)
#define SECONDS_IN_HOUR       (60 * 60)
#define SECONDS_IN_MINUTE     60


static uint32_t mGoToSleepTicks   __attribute__((section(".ret_sram"))) __attribute__((used))  = UINT32_MAX;

uint32_t GetLastTickBeforeDeepSleep(void)
{
  /* mGoToSleepTicks is only written to before the scheduler
   * is started. I.e. it is safe to call this function from any thread
   * after the scheduler is started. */
  return mGoToSleepTicks;
}

static void rtc_isr_func(uint32_t status)
{
  // dummy isr
  (void)status;
}

// Check if a given year is a leap year
static bool is_leap_year(uint32_t year) {
    return ((year % 4 == 0 ) && ((year % 100 != 0) || (year % 400 == 0)));
}

// Get the number of days in a given month of a specific year
static int days_in_month(int year, int month) {
    int daysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return daysPerMonth[month - 1];
}

// Convert milliseconds since the Unix epoch into a date
static void convert_ms_to_date(uint32_t milliseconds, rtc_time_t *date) {
    date->tm_year = 0;
    date->tm_mon = 1;
    date->tm_day = 1;
    date->tm_msec = milliseconds % MS_IN_SECOND;
    milliseconds -= date->tm_msec ;
    // Extract total seconds from milliseconds
    uint32_t totalSeconds = milliseconds / MS_IN_SECOND;
    if (!totalSeconds)
    {
      return;
    }
    // Calculate years
    while (true) {
        uint32_t daysInYear = is_leap_year(date->tm_year) ? 366 : 365;
        if (totalSeconds >= daysInYear * SECONDS_IN_DAY) {
            totalSeconds -= daysInYear * SECONDS_IN_DAY;
            date->tm_year++;
        } else {
            break;
        }
    }

    // Calculate months
    while (true) {
        uint32_t daysInCurrentMonth = days_in_month(date->tm_year, date->tm_mon);
        if (totalSeconds >= daysInCurrentMonth * SECONDS_IN_DAY) {
            totalSeconds -= daysInCurrentMonth * SECONDS_IN_DAY;
            date->tm_mon++;
        } else {
            break;
        }
    }

    // Calculate day, hour, minute, and second
    date->tm_day = totalSeconds / (SECONDS_IN_DAY) + 1;
    totalSeconds %= (SECONDS_IN_DAY);
    date->tm_hour = totalSeconds / SECONDS_IN_HOUR;
    totalSeconds %= SECONDS_IN_HOUR;
    date->tm_min = totalSeconds / SECONDS_IN_MINUTE;
    date->tm_sec = totalSeconds % SECONDS_IN_MINUTE;
}

// Convert the given date into milliseconds since Unix epoch
static uint32_t convert_data_to_ms(rtc_time_t date) {
   uint32_t totalDays = 0;

    // Calculate the total number of days from 1970 to the given year
    for (uint32_t y = 0; y < date.tm_year; y++) {
        totalDays += is_leap_year(y) ? 366 : 365;
    }

    // Add the days of the current year up to the given month
    for (uint32_t m = 1; m < date.tm_mon; m++) {
        totalDays += days_in_month(date.tm_year, m);
    }

    // Add the days of the current month
    totalDays += date.tm_day - 1;

    // Convert total days, hours, minutes, and seconds to milliseconds
    uint32_t milliseconds = totalDays * SECONDS_IN_DAY * 1000;
    milliseconds += date.tm_hour * SECONDS_IN_HOUR * 1000;
    milliseconds += date.tm_min * SECONDS_IN_MINUTE * 1000;
    milliseconds += date.tm_sec * 1000;
    milliseconds += date.tm_msec;

    return milliseconds;
}

void setup_rtc_alarm(uint32_t time_in_ms)
{
  rtc_time_t rtc_alarm = {.tm_year = 0,
                          .tm_mon  = 1,
                          .tm_day  = 1,
                          .tm_hour = 0,
                          .tm_min  = 0,
                          .tm_sec  = 0,
                          .tm_msec = 0};

  rtc_time_t rtc_time ;
  RTC->RTC_CONTROL |= (0x01 << 6); // clear events
  Rtc_Get_Time(&rtc_time);
  mGoToSleepTicks = convert_data_to_ms(rtc_time);
  convert_ms_to_date(time_in_ms + mGoToSleepTicks, &rtc_alarm);

  Rtc_Set_Alarm(&rtc_alarm, RTC_MODE_EVENT_INTERRUPT, rtc_isr_func);
}

uint32_t get_rtc_time(void)
{
  rtc_time_t current_time;
  Rtc_Get_Time(&current_time);
  uint32_t time_in_ms = convert_data_to_ms(current_time);
  return time_in_ms;
}

uint32_t get_rtc_duration(uint32_t current_time)
{
  if (current_time >= GetLastTickBeforeDeepSleep())
  {
    return (current_time - GetLastTickBeforeDeepSleep());
  }
  else
  {
    return (GetLastTickBeforeDeepSleep() + (0xFFFFFFFF - current_time) +1);
  }
}
