/// ****************************************************************************
/// @file T32CM11_rtc.h
///
/// @brief This is the chip specific include file for T32CM11 RTC Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// the RTC module keeps track of 6 time units:
///     seconds,
///     minutes,
///     hours,
///     days,
///     months,
///     and years.
///
/// The RTC module also supports events (interrupts) triggered on time units
/// There are 3 types of events (interrupts) that can be set for these time units:
///
/// 1. ON_UNIT_CHANGE - whenever a time unit changes, get an event. For instance,
///    if this is set for days, then each time the day value changes (a new day
///    starts) then an event would be generated. One of these events can be set
///    for each time unit. (in other words, there could be 6 of these events
///    setup at one time, one event for each time unit)
///
/// 2. ON_SPECIFIC_VALUE - whenever a time unit gets to a specific value. For
///    instance, if this was set for minutes=15, then this event would trigger
///    at times 0:15:00, 1:15:00, 2:15:00, etc. One of these events can be set
///    for each time unit.
///
/// 3. COMBO EVENT - this event is set for a specific second, minute, hour, day,
///    month, year. If this event is set, it is the only event that can be set
///    (no ON_UNIT_CHANGE and no ON_SPECIFIC_VALUE can also be set at the same
///    time). This event can be set so some time units are ignored, but if a
///    time unit is ignored, all time units greater than the ignored time unit
///    must also be ignored. For instance, days can be ignored but then months
///    and years must ALSO be ignored (and sec, min, hours are NOT ignored)
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************


#ifndef T32CM11_RTC_H_
#define T32CM11_RTC_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_rtc_cm11 RTC CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


/// *****************************************************************************
/// \brief min and max values for the six units of time/date
/// *****************************************************************************
#define TR_HAL_RTC_YEAR_MINIMUM 2000
#define TR_HAL_RTC_YEAR_MAXIMUM 2099

#define TR_HAL_RTC_MONTH_MINIMUM  1
#define TR_HAL_RTC_MONTH_MAXIMUM 12

#define TR_HAL_RTC_DAY_MINIMUM  1
#define TR_HAL_RTC_DAY_MAXIMUM 31

#define TR_HAL_RTC_SHORT_MONTH_DAY_MAXIMUM   30
#define TR_HAL_RTC_FEB_DAY_MAXIMUM           28
#define TR_HAL_RTC_FEB_LEAP_YEAR_DAY_MAXIMUM 29

#define TR_HAL_RTC_HOUR_MINIMUM  0
#define TR_HAL_RTC_HOUR_MAXIMUM 23

#define TR_HAL_RTC_MINUTE_MINIMUM  0
#define TR_HAL_RTC_MINUTE_MAXIMUM 59

#define TR_HAL_RTC_SECOND_MINIMUM  0
#define TR_HAL_RTC_SECOND_MAXIMUM 59


/// ****************************************************************************
/// defines for months
/// ****************************************************************************
#define TR_HAL_MONTH_JANUARY    1
#define TR_HAL_MONTH_FEBRUARY   2
#define TR_HAL_MONTH_MARCH      3
#define TR_HAL_MONTH_APRIL      4
#define TR_HAL_MONTH_MAY        5
#define TR_HAL_MONTH_JUNE       6
#define TR_HAL_MONTH_JULY       7
#define TR_HAL_MONTH_AUGUST     8
#define TR_HAL_MONTH_SEPTEMBER  9
#define TR_HAL_MONTH_OCTOBER   10
#define TR_HAL_MONTH_NOVEMBER  11
#define TR_HAL_MONTH_DECEMBER  12


/// *****************************************************************************
/// \brief convenience structs for use when passing around dates, times, or both
/// *****************************************************************************

// date
typedef struct
{
    // 4 digit year and must be >= 2000
    uint16_t years;

    // 1-12
    uint8_t  months;

    // 1-31, depends on month
    uint8_t  days;

} tr_hal_rtc_date;

// time
typedef struct
{
    // range 0-23
    uint8_t hours;

    // range 0-59
    uint8_t minutes;

    // range 0-59
    uint8_t seconds;

} tr_hal_rtc_time;

// date and time
typedef struct
{
    tr_hal_rtc_date date;
    tr_hal_rtc_time time;

} tr_hal_rtc_date_time;


/// *****************************************************************************
/// \brief special invalid value for any of the 6 time units:
/// this define is used for the COMBO EVENT. If the caller does not want to
/// use one or more of the time units, then the value of those time units
/// should be set to this value
/// *****************************************************************************
#define TR_HAL_INVALID_DATE_TIME_VALUE 255


/// *****************************************************************************
/// \brief this enum is used when we set up a time unit event, to tell which
/// units we are using
/// *****************************************************************************
typedef enum
{
    TR_HAL_RTC_TIME_UNIT_SECONDS = 0,
    TR_HAL_RTC_TIME_UNIT_MINUTES = 1,
    TR_HAL_RTC_TIME_UNIT_HOURS   = 2,
    TR_HAL_RTC_TIME_UNIT_DAYS    = 3,
    TR_HAL_RTC_TIME_UNIT_MONTHS  = 4,
    TR_HAL_RTC_TIME_UNIT_YEARS   = 5,
    TR_HAL_RTC_TIME_UNIT_INVALID = 255,

} tr_hal_rtc_time_unit_t;


/// *****************************************************************************
/// \brief this enum is the trigger setting for time unit events
/// OFF = not enabled
/// ON_CHANGE = whenever the time unit changes
/// ON_VALUE = only when time unit == a specific value
/// INVALID = used when setting up a combo event when a time unit should not be used
/// *****************************************************************************
typedef enum
{
    TR_HAL_EVENT_TRIGGER_OFF               = 0,
    TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE    = 1,
    TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE = 2,
    TR_HAL_EVENT_TRIGGER_INVALID           = 255,

} tr_hal_rtc_event_trigger_t;


/// ******************************************************************
/// \brief chip register addresses
/// section 3.1 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts and configures
/// the SPI peripherals. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base address
/// and a struct field to read or write a chip register
/// ******************************************************************
#define CHIP_MEMORY_MAP_RTC_BASE     (0x40600000UL)


/// ***************************************************************************
/// \brief the struct we use so we can address registers using field names
/// ***************************************************************************
typedef struct
{
    // register values for seconds, minutes, hours, days, months, years
    // note that these must be written and read in BCD
    __IO  uint32_t   seconds_value;         // 0x00
    __IO  uint32_t   minutes_value;         // 0x04
    __IO  uint32_t   hours_value;           // 0x08
    __IO  uint32_t   days_value;            // 0x0C
    __IO  uint32_t   months_value;          // 0x10
    __IO  uint32_t   years_value;           // 0x14

    // used for testing (can speed up rate) and to clear counters and values
    __IO  uint32_t   control;               // 0x18

    // Clock Divisor for generating a 1Hz enable pulse to the seconds counter
    __IO  uint32_t   clock_divisor;         // 0x1C

    // what trigger to use for each time unit: off, on_change, on_value, combo
    // these registers also hold the trigger value if in on_value mode
    __IO  uint32_t   seconds_event_trigger; // 0x20
    __IO  uint32_t   minutes_event_trigger; // 0x24
    __IO  uint32_t   hours_event_trigger;   // 0x28
    __IO  uint32_t   days_event_trigger;    // 0x2C
    __IO  uint32_t   months_event_trigger;  // 0x30
    __IO  uint32_t   years_event_trigger;   // 0x34

    // interrupts
    __IO  uint32_t   interrupt_enable;      // 0x38
    __IO  uint32_t   interrupt_status;      // 0x3C
    __IO  uint32_t   interrupt_clear;       // 0x40

    // once clock divisior, event trigger values, or time values have been updated,
    // this register allows them to be "loaded" == become active
    __IO  uint32_t   load;                  // 0x44

} RTC_REGISTERS_T;


// *****************************************************************
// this enum helps when dealing with the CONTROL REGISTER (0x18)
typedef enum
{
    TR_HAL_RTC_SPEEDUP_NONE    = 0x01,
    TR_HAL_RTC_SPEEDUP_MINUTES = 0x02,
    TR_HAL_RTC_SPEEDUP_HOURS   = 0x04,
    TR_HAL_RTC_SPEEDUP_DAYS    = 0x08,
    TR_HAL_RTC_SPEEDUP_MONTHS  = 0x10,
    TR_HAL_RTC_SPEEDUP_YEARS   = 0x20,

} tr_hal_rtc_speedup_unit_t;


// *****************************************************************
// these defines help when dealing with the CLOCK DIVISIOR REGISTER (0x1C)
// we expect a 32 KHz clock
#define TR_HAL_DEFAULT_CLOCK_DIVISOR 39999


// *****************************************************************
// these defines help when dealing with the EVENT TRIGGER REGISTERs (0x20 - 0x34)
#define TR_HAL_REG_SETTING_EVENT_OFF       0x0000
#define TR_HAL_REG_SETTING_EVENT_ON_CHANGE 0x0100
#define TR_HAL_REG_SETTING_EVENT_ON_VALUE  0x0000
#define TR_HAL_REG_SETTING_COMBO_EVENT     0x0200


// *****************************************************************
// these defines help when dealing with the INTERRUPT ENABLE REGISTER (0x38),
// the INTERRUPT STATUS REGISTER (0x3C) and the INTERRUPT CLEAR REGISTER (0x40)
#define TR_HAL_RTC_INTERRUPT_NONE             (uint32_t)0x00
#define TR_HAL_RTC_INTERRUPT_SECONDS          (uint32_t)0x01
#define TR_HAL_RTC_INTERRUPT_MINUTES          (uint32_t)0x02
#define TR_HAL_RTC_INTERRUPT_HOURS            (uint32_t)0x04
#define TR_HAL_RTC_INTERRUPT_DAYS             (uint32_t)0x08
#define TR_HAL_RTC_INTERRUPT_MONTHS           (uint32_t)0x10
#define TR_HAL_RTC_INTERRUPT_YEARS            (uint32_t)0x20
#define TR_HAL_RTC_INTERRUPT_COMBINED_EVENT   (uint32_t)0x40
#define TR_HAL_RTC_INTERRUPT_ALL              (uint32_t)0x7F


// *****************************************************************
// these defines help when dealing with the LOAD REGISTER (0x44)
#define TR_HAL_RTC_UPDATE_TIME_VALUES   0x01
#define TR_HAL_RTC_UPDATE_EVENT_TRIGGER 0x02
#define TR_HAL_RTC_UPDATE_CLOCK_DIVISOR 0x04


// *****************************************************************
// this orients the RTC_REGISTERS struct with the correct address
// so referencing a field will now read/write the correct RTC
// register chip address
#define RTC_REGISTERS  ((RTC_REGISTERS_T *) CHIP_MEMORY_MAP_RTC_BASE)


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of the RTC registers
/// ***************************************************************************
RTC_REGISTERS_T* tr_hal_rtc_get_register_address(void);


/// ***************************************************************************
/// prototype for callback from the Trident HAL to the app when an event happens
/// ***************************************************************************
typedef void (*tr_hal_rtc_event_callback_t) (uint32_t event_bitmask,
                                             tr_hal_rtc_date_time current_date_time);


/// ***************************************************************************
/// these are the EVENTS that can be received into the RTC event handler
/// functions. These are BITMASKs since we can have more than 1 in an event
/// these are what the APP needs to handle in its event_handler_fx
/// ***************************************************************************
#define TR_HAL_RTC_EVENT_TRIGGERED_SECONDS          0x00000001
#define TR_HAL_RTC_EVENT_TRIGGERED_MINUTES          0x00000002
#define TR_HAL_RTC_EVENT_TRIGGERED_HOURS            0x00000004
#define TR_HAL_RTC_EVENT_TRIGGERED_DAYS             0x00000008
#define TR_HAL_RTC_EVENT_TRIGGERED_MONTHS           0x00000010
#define TR_HAL_RTC_EVENT_TRIGGERED_YEARS            0x00000020
#define TR_HAL_RTC_EVENT_TRIGGERED_COMBINED_EVENT   0x00000040


/// ***************************************************************************
/// RTC settings struct - this is passed to tr_hal_rtc_init
///
/// ***************************************************************************
typedef struct
{
    // **** date, time, and clock divisor ****

    tr_hal_rtc_date_time rtc_date_time;

    uint32_t clock_divisor;


    // **** event callback from HAL to App when an event happens ****
    // if the app doesn't want this, then set it to NULL
    tr_hal_rtc_event_callback_t  event_handler_fx;


    // **** chip behavior settings ****

    // are the chip interrupts enabled?
    bool enable_chip_interrupts;

    // set the INT priority
    tr_hal_int_pri_t interrupt_priority;

    // when the device is sleeping, we can choose to DISABLE interrupts,
    // or leave them enabled which would allow the device to wake on
    // an interrupt from this peripheral
    bool wake_on_interrupt;


    // **** time unit events ****

    tr_hal_rtc_event_trigger_t seconds_event_trigger;
    uint8_t                    seconds_trigger_value;
    tr_hal_rtc_event_trigger_t minutes_event_trigger;
    uint8_t                    minutes_trigger_value;
    tr_hal_rtc_event_trigger_t hours_event_trigger;
    uint8_t                    hours_trigger_value;
    tr_hal_rtc_event_trigger_t days_event_trigger;
    uint8_t                    days_trigger_value;
    tr_hal_rtc_event_trigger_t months_event_trigger;
    uint8_t                    months_trigger_value;
    tr_hal_rtc_event_trigger_t years_event_trigger;
    uint16_t                   years_trigger_value;


    // **** combo event ****

    bool                      combo_event_enabled;
    tr_hal_rtc_date_time      combo_event_trigger_date_time;

} tr_hal_rtc_settings_t;


/// ***************************************************************************
/// default RTC settings
///
/// date/time: Jan 1, 2025 at midnight (about to be 12:01 am on Jan 1)
/// clock is set for 32 KHz (this could be tuned and based on an NV setting)
/// event handler should be set by the app
/// interrupts are enabled, with a reasonable priority, and are disabled on power off
/// ***************************************************************************
#define DEFAULT_RTC_CONFIG                                  \
    {                                                       \
        .rtc_date_time.time.seconds =  0,                   \
        .rtc_date_time.time.minutes =  0,                   \
        .rtc_date_time.time.hours =  0,                     \
        .rtc_date_time.date.days =  1,                      \
        .rtc_date_time.date.months =  1,                    \
        .rtc_date_time.date.years =  2025,                  \
        .clock_divisor =  TR_HAL_DEFAULT_CLOCK_DIVISOR,     \
        .event_handler_fx =  NULL,                          \
        .enable_chip_interrupts =  true,                    \
        .interrupt_priority =  TR_HAL_INTERRUPT_PRIORITY_5, \
        .wake_on_interrupt =  false,                        \
        .seconds_event_trigger = TR_HAL_EVENT_TRIGGER_OFF,  \
        .minutes_event_trigger = TR_HAL_EVENT_TRIGGER_OFF,  \
        .hours_event_trigger   = TR_HAL_EVENT_TRIGGER_OFF,  \
        .days_event_trigger    = TR_HAL_EVENT_TRIGGER_OFF,  \
        .months_event_trigger  = TR_HAL_EVENT_TRIGGER_OFF,  \
        .years_event_trigger   = TR_HAL_EVENT_TRIGGER_OFF,  \
        .combo_event_enabled = false,                       \
    }


// this API causes the time unit specified to speed up to change once per second
// this is useful for testing
tr_hal_status_t tr_hal_rtc_speedup_for_testing(tr_hal_rtc_speedup_unit_t speedup_unit);

// expose these so they can be fully tested
uint32_t convert_bcd_to_int(uint32_t register_value);
uint32_t convert_int_to_bcd(uint32_t int_value);


/// ****************************************************************************
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_RTC_H_
