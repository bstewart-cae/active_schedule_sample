/// ****************************************************************************
/// @file tr_hal_rtc.c
///
/// @brief This contains the code for the Trident HAL RTC for T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_rtc.h"


/// ***************************************************************************
/// static variables
/// ***************************************************************************

// callback to the app to use for events
static tr_hal_rtc_event_callback_t g_rtc_callback = NULL;

// desired interrupt behavior
static bool g_rtc_enable_chip_interrupts = true;
static tr_hal_int_pri_t g_rtc_interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5;
static bool g_rtc_wake_on_interrupt = false;

// this code needs to keep track of if a combo event has been configured
// there isn't a 100 percent accurate way (that I can tell) to tell if this
// is the case from the chip. And this is much simpler than reading 6 registers
// and doing some calculations
bool g_combo_event_active = false;

// when the init function runs, we need to NOT write to the load register
// when it calls other functions, we need to wait until ALL the writing is
// done before setting the load register. This allows that to work and
// when APIs are called directly they will still work.
bool g_rtc_ok_to_run_load = true;


/// ***************************************************************************
/// tr_hal_rtc_get_register_address
/// ***************************************************************************
RTC_REGISTERS_T* tr_hal_rtc_get_register_address(void)
{
    return RTC_REGISTERS;
}


/// ***************************************************************************
/// check_time_valid
/// ***************************************************************************
static tr_hal_status_t check_time_valid(tr_hal_rtc_time check_time)
{
    if (check_time.hours > TR_HAL_RTC_HOUR_MAXIMUM)
    {
        return TR_HAL_RTC_INVALID_HOUR;
    }

    if (check_time.minutes > TR_HAL_RTC_MINUTE_MAXIMUM)
    {
        return TR_HAL_RTC_INVALID_MINUTE;
    }

    if (check_time.seconds > TR_HAL_RTC_SECOND_MAXIMUM)
    {
        return TR_HAL_RTC_INVALID_SECOND;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// check_date_valid
/// ***************************************************************************
static tr_hal_status_t check_date_valid(tr_hal_rtc_date check_date)
{
    // get the values
    uint16_t years = check_date.years;
    uint8_t months = check_date.months;
    uint8_t days   = check_date.days;

    if ((years > TR_HAL_RTC_YEAR_MAXIMUM)
        || (years < TR_HAL_RTC_YEAR_MINIMUM))
    {
        return TR_HAL_RTC_INVALID_YEAR;
    }

    if ((months > TR_HAL_RTC_MONTH_MAXIMUM)
        || (months < TR_HAL_RTC_MONTH_MINIMUM))
    {
        return TR_HAL_RTC_INVALID_MONTH;
    }

    if ((days > TR_HAL_RTC_DAY_MAXIMUM)
        || (days < TR_HAL_RTC_DAY_MINIMUM))
    {
        return TR_HAL_RTC_INVALID_DAY;
    }

    // check the special cases of months with 30 days
    if (   (months == TR_HAL_MONTH_APRIL)
        || (months == TR_HAL_MONTH_JUNE)
        || (months == TR_HAL_MONTH_SEPTEMBER)
        || (months == TR_HAL_MONTH_NOVEMBER)
        )
    {
        if (days > TR_HAL_RTC_SHORT_MONTH_DAY_MAXIMUM)
        {
            return TR_HAL_RTC_INVALID_DAY;
        }
    }

    // check the special case of February
    if (months == TR_HAL_MONTH_FEBRUARY)
    {
        uint16_t max_feb_day = TR_HAL_RTC_FEB_DAY_MAXIMUM;
        // if leap year
        // (we only account for one every 4 years and don't worry about the
        // not-29-days on a year divisible by 100, since the next year is 2100)
        if ((years % 4) == 0)
        {
            max_feb_day = TR_HAL_RTC_FEB_LEAP_YEAR_DAY_MAXIMUM;
        }

        if (days > max_feb_day)
        {
            return TR_HAL_RTC_INVALID_DAY;
        }
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// convert a BCD register value to an int
/// bits 0-3 are the ones place =  mask 0x0F
/// bits 4-7 are the tens place = mask 0xF0
/// ***************************************************************************
uint32_t convert_bcd_to_int(uint32_t register_value)
{
    uint32_t ones = register_value & 0x0F;
    uint32_t tens = (register_value >> 4);

    uint32_t return_value = ones + (tens * 10);

    return return_value;
}


/// ***************************************************************************
/// convert an INT into a BCD number so it can be stored in the register
/// bits 0-3 are the ones place =  mask 0x0F
/// bits 4-7 are the tens place = mask 0xF0
/// ***************************************************************************
uint32_t convert_int_to_bcd(uint32_t int_value)
{
    // we can only convert up to 2 digits, meaning a max of 99
    uint32_t int_value_checked = int_value % 100;

    // split tens and ones
    uint32_t int_value_tens = int_value_checked / 10;
    uint32_t int_value_ones = int_value_checked % 10;

    uint32_t bcd_value = (int_value_tens << 4) + int_value_ones;

    return bcd_value;
}


/// ***************************************************************************
/// tr_hal_rtc_init
///
/// this is the perferred method for setting up the RTC. Fill in a
/// tr_hal_rtc_settings_t struct and pass it to this function
///
/// this can be used to set:
///   - the time
///   - the date
///   - the clock divisor
///   - the event handler function to use
///   - interrupt settings
///   - time unit events
///   - combo event
///
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_init(tr_hal_rtc_settings_t* rtc_settings)
{
    // settings can't be NULL
    if (rtc_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // error check: can't have a combo event AND a time unit event
    if ( (rtc_settings->combo_event_enabled)
        &&
          (  (rtc_settings->seconds_event_trigger != TR_HAL_EVENT_TRIGGER_OFF)
          || (rtc_settings->minutes_event_trigger != TR_HAL_EVENT_TRIGGER_OFF)
          || (rtc_settings->hours_event_trigger != TR_HAL_EVENT_TRIGGER_OFF)
          || (rtc_settings->months_event_trigger != TR_HAL_EVENT_TRIGGER_OFF)
          || (rtc_settings->days_event_trigger != TR_HAL_EVENT_TRIGGER_OFF)
          || (rtc_settings->years_event_trigger != TR_HAL_EVENT_TRIGGER_OFF)
          )
       )
    {
        return TR_HAL_RTC_EVENT_CONFLICT;
    }

    // we don't want the individual functions doing a load until we are all done
    g_rtc_ok_to_run_load = false;

    // get the chip register address
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();
    tr_hal_status_t status;

    // set the date and time
    status = tr_hal_rtc_set_date_time(rtc_settings->rtc_date_time);
    if (status != TR_HAL_SUCCESS)
    {
        g_rtc_ok_to_run_load = true;
        return status;
    }

    // set the clock divisor
    status = tr_hal_rtc_set_clock_divisor(rtc_settings->clock_divisor);
    if (status != TR_HAL_SUCCESS)
    {
        g_rtc_ok_to_run_load = true;
        return status;
    }

    // remember the callback so it can be used later
    g_rtc_callback = rtc_settings->event_handler_fx;

    // check that interrupt_priority is in the correct range
    status = tr_hal_check_interrupt_priority(rtc_settings->interrupt_priority);
    if (status != TR_HAL_SUCCESS)
    {
        g_rtc_ok_to_run_load = true;
        return status;
    }

    // set the interrupt settings
    g_rtc_enable_chip_interrupts = rtc_settings->enable_chip_interrupts;
    g_rtc_interrupt_priority = rtc_settings->interrupt_priority;
    g_rtc_wake_on_interrupt = rtc_settings->wake_on_interrupt;

    // we DO NOT enable specific interrupts until we have an event set for one of them
    rtc_reg->interrupt_enable = TR_HAL_RTC_INTERRUPT_NONE;

    // we clear ALL interrupts in case there were any pending
    rtc_reg->interrupt_clear = TR_HAL_RTC_INTERRUPT_ALL;

    // check for combo event
    if (rtc_settings->combo_event_enabled)
    {
        status = tr_hal_rtc_set_combo_trigger_event(rtc_settings->combo_event_trigger_date_time);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }
    }
    // check for time unit event
    else
    {
        // seconds
        status = tr_hal_rtc_set_time_trigger_event(TR_HAL_RTC_TIME_UNIT_SECONDS,
                                                   rtc_settings->seconds_event_trigger,
                                                   rtc_settings->seconds_trigger_value);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }

        // minutes
        status = tr_hal_rtc_set_time_trigger_event(TR_HAL_RTC_TIME_UNIT_MINUTES,
                                                   rtc_settings->minutes_event_trigger,
                                                   rtc_settings->minutes_trigger_value);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }

        // hours
        status = tr_hal_rtc_set_time_trigger_event(TR_HAL_RTC_TIME_UNIT_HOURS,
                                                   rtc_settings->hours_event_trigger,
                                                   rtc_settings->hours_trigger_value);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }

        // days
        status = tr_hal_rtc_set_time_trigger_event(TR_HAL_RTC_TIME_UNIT_DAYS,
                                                   rtc_settings->days_event_trigger,
                                                   rtc_settings->days_trigger_value);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }

        // months
        status = tr_hal_rtc_set_time_trigger_event(TR_HAL_RTC_TIME_UNIT_MONTHS,
                                                   rtc_settings->months_event_trigger,
                                                   rtc_settings->months_trigger_value);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }

        // years
        status = tr_hal_rtc_set_time_trigger_event(TR_HAL_RTC_TIME_UNIT_YEARS,
                                                   rtc_settings->years_event_trigger,
                                                   rtc_settings->years_trigger_value);
        if (status != TR_HAL_SUCCESS)
        {
            g_rtc_ok_to_run_load = true;
            return status;
        }
    }


    // enable or disable NVIC interrupts
    if (g_rtc_enable_chip_interrupts)
    {
        // enable interrupts at the chip
        NVIC_SetPriority(Rtc_IRQn, g_rtc_interrupt_priority);
        NVIC_EnableIRQ(Rtc_IRQn);
    }
    else
    {
        NVIC_DisableIRQ(Rtc_IRQn);
    }


    // make sure the speedup (test) is UNSET
    rtc_reg->control = TR_HAL_RTC_SPEEDUP_NONE;

    // this tells the chip we are done updating the time/date values, the
    // clock divisor, and the time unit events (or combo event) and to
    // load the values written
    rtc_reg->load = TR_HAL_RTC_UPDATE_TIME_VALUES
                  | TR_HAL_RTC_UPDATE_EVENT_TRIGGER
                  | TR_HAL_RTC_UPDATE_CLOCK_DIVISOR;

    // ok to load now
    g_rtc_ok_to_run_load = true;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_rtc_is_running
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_is_running(bool* is_running)
{
    // on the CM11 the RTC starts up automatically, even if
    // no code initializes it
    (*is_running) = true;

    return TR_HAL_SUCCESS;
}

/// ***************************************************************************
/// set time
///
/// this sets the time by directly writing to the registers
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_time(tr_hal_rtc_time new_time)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // check that the time is valid
    tr_hal_status_t status = check_time_valid(new_time);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // write the time into the registers
    rtc_reg->seconds_value = convert_int_to_bcd(new_time.seconds);
    rtc_reg->minutes_value = convert_int_to_bcd(new_time.minutes);
    rtc_reg->hours_value   = convert_int_to_bcd(new_time.hours);

    // this tells the chip we are done updating the time/date values
    // and to load the values written
    if (g_rtc_ok_to_run_load)
    {
        rtc_reg->load = TR_HAL_RTC_UPDATE_TIME_VALUES;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// set date
///
/// this sets the date by directly writing to the registers
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_date(tr_hal_rtc_date new_date)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // check that the date is valid
    tr_hal_status_t status = check_date_valid(new_date);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // write the date into the registers
    rtc_reg->days_value   = convert_int_to_bcd(new_date.days);
    rtc_reg->months_value = convert_int_to_bcd(new_date.months);
    rtc_reg->years_value  = convert_int_to_bcd(new_date.years - 2000);


    // this tells the chip we are done updating the time/date values
    // and to load the values written
    if (g_rtc_ok_to_run_load)
    {
        rtc_reg->load = TR_HAL_RTC_UPDATE_TIME_VALUES;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// set time and date
///
/// note: we don't use the set_date/set_time APIs since we want to check both
/// date and time for validity before setting either. Also this allows
/// the code to set the load register once, rather than twice on setting
/// the date and then the time.
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_date_time(tr_hal_rtc_date_time new_date_time)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();
    tr_hal_status_t status;

    // check that the time is valid
    status = check_time_valid(new_date_time.time);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check that the date is valid
    status = check_date_valid(new_date_time.date);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // write the time into the registers
    rtc_reg->seconds_value = convert_int_to_bcd(new_date_time.time.seconds);
    rtc_reg->minutes_value = convert_int_to_bcd(new_date_time.time.minutes);
    rtc_reg->hours_value   = convert_int_to_bcd(new_date_time.time.hours);

    // write the date into the registers
    rtc_reg->days_value   = convert_int_to_bcd(new_date_time.date.days);
    rtc_reg->months_value = convert_int_to_bcd(new_date_time.date.months);
    rtc_reg->years_value  = convert_int_to_bcd(new_date_time.date.years - 2000);

    // this tells the chip we are done updating the time/date values
    // and to load the values written
    if (g_rtc_ok_to_run_load)
    {
        rtc_reg->load = TR_HAL_RTC_UPDATE_TIME_VALUES;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// set RTC event handler
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_event_handler(tr_hal_rtc_event_callback_t new_event_handler)
{
    if (new_event_handler != NULL)
    {
        g_rtc_callback = new_event_handler;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// set RTC interrupt handling
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_interrupt_behavior(bool             enable_chip_interrupts,
                                                  tr_hal_int_pri_t interrupt_priority,
                                                  bool             wake_on_interrupt)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();
    tr_hal_status_t status;

    // check that interrupt_priority is in the correct range
    status = tr_hal_check_interrupt_priority(interrupt_priority);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set the interrupt settings
    g_rtc_enable_chip_interrupts = enable_chip_interrupts;
    g_rtc_interrupt_priority = interrupt_priority;
    g_rtc_wake_on_interrupt = wake_on_interrupt;

    // we DO NOT enable specific interrupts until we have an event set for one of them
    rtc_reg->interrupt_enable = TR_HAL_RTC_INTERRUPT_NONE;

    // we clear ALL interrupts in case there were any pending
    rtc_reg->interrupt_clear = TR_HAL_RTC_INTERRUPT_ALL;

    // enable or disable NVIC interrupts
    if (g_rtc_enable_chip_interrupts)
    {
        // enable interrupts at the chip
        NVIC_SetPriority(Rtc_IRQn, g_rtc_interrupt_priority);
        NVIC_EnableIRQ(Rtc_IRQn);
    }
    else
    {
        NVIC_DisableIRQ(Rtc_IRQn);
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// get RTC interrupt handling
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_get_interrupt_behavior(bool*             enable_chip_interrupts,
                                                  tr_hal_int_pri_t* interrupt_priority,
                                                  bool*             wake_on_interrupt)
{
    (*enable_chip_interrupts) = g_rtc_enable_chip_interrupts;
    (*interrupt_priority) = g_rtc_interrupt_priority;
    (*wake_on_interrupt) = g_rtc_wake_on_interrupt;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// get current time
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_get_time(tr_hal_rtc_time* return_time)
{
    // return value can't be NULL
    if (return_time == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // note: there is no check needed to see if the RTC has already been
    // started. It starts after the chip boots

    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // get the time from the chip registers
    uint32_t seconds_register_value = rtc_reg->seconds_value;
    uint32_t minutes_register_value = rtc_reg->minutes_value;
    uint32_t hours_register_value = rtc_reg->hours_value;

    // each of these needs to be converted from BCD to int
    return_time->seconds = (uint8_t) convert_bcd_to_int(seconds_register_value);
    return_time->minutes = (uint8_t) convert_bcd_to_int(minutes_register_value);
    return_time->hours   = (uint8_t) convert_bcd_to_int(hours_register_value);

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// get current date
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_get_date(tr_hal_rtc_date* return_date)
{
    // return value can't be NULL
    if (return_date == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // note: there is no check needed to see if the RTC has already been
    // started. It starts after the chip boots

    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // get the date from the chip registers
    uint32_t days_register_value = rtc_reg->days_value;
    uint32_t months_register_value = rtc_reg->months_value;
    uint32_t years_register_value = rtc_reg->years_value;

    // each of these needs to be converted from BCD to int
    return_date->days    = (uint8_t) convert_bcd_to_int(days_register_value);
    return_date->months  = (uint8_t) convert_bcd_to_int(months_register_value);
    // the chip stores a 2-digit year, the software keeps a 4-digit year
    // doesn't anyone remember Y2K?
    return_date->years   = (uint16_t) (2000 + convert_bcd_to_int(years_register_value));

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// get current time and date
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_get_date_time(tr_hal_rtc_date_time* return_date_time)
{
    // return value can't be NULL
    if (return_date_time == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // note: there is no check needed to see if the RTC has already been
    // started. It starts after the chip boots

    // we don't check status code, since we already checked for NULL and that is
    // the only error from these functions
    tr_hal_rtc_get_time(&(return_date_time->time));
    tr_hal_rtc_get_date(&(return_date_time->date));

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_rtc_diff_greater_than_ms
///
/// max value for time is 23 hours, 59 mins, 59 seconds, 9999ms
/// 24 hours x 3600 seconds x 1000 = 86,400,000 = 0x05265C00 => fits in int32u
/// ***************************************************************************
bool tr_hal_rtc_diff_greater_than_ms(tr_hal_rtc_time* time1,
                                     tr_hal_rtc_time* time2,
                                     uint32_t check_against_ms)
{
    uint32_t larger_seconds = (uint32_t)((time1->hours * 3600) + (time1->minutes * 60) + time1->seconds);
    uint32_t smaller_seconds = (uint32_t)((time2->hours * 3600) + (time2->minutes * 60) + time2->seconds);

    // if they are equal then result is false
    if (smaller_seconds == larger_seconds)
    {
        return false;
    }

    // find the diff - absolute value
    uint32_t diff = 0;
    if (smaller_seconds > larger_seconds)
    {
        diff = smaller_seconds - larger_seconds;
    }
    else
    {
        diff = larger_seconds - smaller_seconds;
    }

    // adjust for ms
    diff = diff * 1000;

    // is diff larger than compare time?
    if (diff >= check_against_ms)
    {
        return true;
    }
    // else
    return false;
}


/// ***************************************************************************
/// tr_hal_rtc_get_clock_divisor
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_get_clock_divisor(uint32_t* clock_divisor)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    (*clock_divisor) = rtc_reg->clock_divisor;

    return TR_HAL_SUCCESS;
}

/// ***************************************************************************
/// tr_hal_rtc_set_clock_divisor
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_clock_divisor(uint32_t clock_divisor)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // set the clock
    rtc_reg->clock_divisor = clock_divisor;

    // this tells the chip we are done updating the clock divisor value
    // and to start using the new clock divisor value
    if (g_rtc_ok_to_run_load)
    {
        rtc_reg->load = TR_HAL_RTC_UPDATE_CLOCK_DIVISOR;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_rtc_set_time_trigger_event
///
/// enable or disable an event trigger for one of the 6 time units
/// can setup: OFF, ON_CHANGE, ON_VALUE
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_time_trigger_event(tr_hal_rtc_time_unit_t     time_unit,
                                                  tr_hal_rtc_event_trigger_t event_trigger,
                                                  uint16_t                   trigger_value)
{
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // if we have a combo event set, we need to disable it, which means to to CLEAR
    // out all event registers and make sure the combo interrupt is disabled
    if (g_combo_event_active)
    {
        // clear ALL event registers of the combo event
        rtc_reg->seconds_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
        rtc_reg->minutes_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
        rtc_reg->hours_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
        rtc_reg->days_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
        rtc_reg->months_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
        rtc_reg->years_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
        // all interrupts need to start off
        rtc_reg->interrupt_enable = TR_HAL_RTC_INTERRUPT_NONE;
        // we are no longer in a combo event
        g_combo_event_active = false;
    }

    // get the address of the register to change and check the trigger_value if necessary
    switch (time_unit)
    {
        // *** seconds ***
        case TR_HAL_RTC_TIME_UNIT_SECONDS:

            // check range if we are setting a value
            if (   (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
                && (trigger_value > TR_HAL_RTC_SECOND_MAXIMUM))
            {
                return TR_HAL_RTC_INVALID_SECOND;
            }
            // seconds trigger = OFF
            if (event_trigger == TR_HAL_EVENT_TRIGGER_OFF)
            {
                // set register and turn off interrupt
                rtc_reg->seconds_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
                rtc_reg->interrupt_enable &= (~TR_HAL_RTC_INTERRUPT_SECONDS);
            }
            // seconds trigger = ON CHANGE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE)
            {
                // set the register and turn on the interrupt
                rtc_reg->seconds_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_CHANGE;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_SECONDS;
            }
            // seconds trigger = ON VALUE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
            {
                // set the register (include the value) and turn on the interrupt
                // convert the value to BCD first]
                uint32_t bcd_trigger_value = convert_int_to_bcd(trigger_value);
                rtc_reg->seconds_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_VALUE | bcd_trigger_value;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_SECONDS;
            }
            break;

        // *** minutes ***
        case TR_HAL_RTC_TIME_UNIT_MINUTES:

            // check range if we are setting a value
            if (   (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
                && (trigger_value > TR_HAL_RTC_MINUTE_MAXIMUM))
            {
                return TR_HAL_RTC_INVALID_MINUTE;
            }
            // minutes trigger = OFF
            if (event_trigger == TR_HAL_EVENT_TRIGGER_OFF)
            {
                // set register and turn off interrupt
                rtc_reg->minutes_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
                rtc_reg->interrupt_enable &= (~TR_HAL_RTC_INTERRUPT_MINUTES);
            }
            // minutes trigger = ON CHANGE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE)
            {
                // set the register and turn on the interrupt
                rtc_reg->minutes_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_CHANGE;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_MINUTES;
            }
            // minutes trigger = ON VALUE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
            {
                // set the register (include the value) and turn on the interrupt
                // convert the value to BCD first]
                uint32_t bcd_trigger_value = convert_int_to_bcd(trigger_value);
                rtc_reg->minutes_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_VALUE | bcd_trigger_value;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_MINUTES;
            }
            break;


        // *** hours ***
        case TR_HAL_RTC_TIME_UNIT_HOURS:

            // check range if we are setting a value
            if (   (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
                && (trigger_value > TR_HAL_RTC_HOUR_MAXIMUM))
            {
                return TR_HAL_RTC_INVALID_HOUR;
            }
            // hours trigger = OFF
            if (event_trigger == TR_HAL_EVENT_TRIGGER_OFF)
            {
                // set register and turn off interrupt
                rtc_reg->hours_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
                rtc_reg->interrupt_enable &= (~TR_HAL_RTC_INTERRUPT_HOURS);
            }
            // hours trigger = ON CHANGE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE)
            {
                // set the register and turn on the interrupt
                rtc_reg->hours_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_CHANGE;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_HOURS;
            }
            // hours trigger = ON VALUE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
            {
                // set the register (include the value) and turn on the interrupt
                // convert the value to BCD first]
                uint32_t bcd_trigger_value = convert_int_to_bcd(trigger_value);
                rtc_reg->hours_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_VALUE | bcd_trigger_value;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_HOURS;
            }
            break;

        // *** days ***
        case TR_HAL_RTC_TIME_UNIT_DAYS:

            // check range if we are setting a value
            if (   (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
                && (  (trigger_value > TR_HAL_RTC_DAY_MAXIMUM)
                  ||  (trigger_value < TR_HAL_RTC_DAY_MINIMUM)))
            {
                return TR_HAL_RTC_INVALID_DAY;
            }
            // days trigger = OFF
            if (event_trigger == TR_HAL_EVENT_TRIGGER_OFF)
            {
                // set register and turn off interrupt
                rtc_reg->days_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
                rtc_reg->interrupt_enable &= (~TR_HAL_RTC_INTERRUPT_DAYS);
            }
            // days trigger = ON CHANGE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE)
            {
                // set the register and turn on the interrupt
                rtc_reg->days_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_CHANGE;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_DAYS;
            }
            // days trigger = ON VALUE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
            {
                // set the register (include the value) and turn on the interrupt
                // convert the value to BCD first]
                uint32_t bcd_trigger_value = convert_int_to_bcd(trigger_value);
                rtc_reg->days_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_VALUE | bcd_trigger_value;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_DAYS;
            }
            break;

        // *** months ***
        case TR_HAL_RTC_TIME_UNIT_MONTHS:

            // check range if we are setting a value
            if (   (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
                && (  (trigger_value > TR_HAL_RTC_MONTH_MAXIMUM)
                  ||  (trigger_value < TR_HAL_RTC_MONTH_MINIMUM)))
            {
                return TR_HAL_RTC_INVALID_MONTH;
            }
            // months trigger = OFF
            if (event_trigger == TR_HAL_EVENT_TRIGGER_OFF)
            {
                // set register and turn off interrupt
                rtc_reg->months_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
                rtc_reg->interrupt_enable &= (~TR_HAL_RTC_INTERRUPT_MONTHS);
            }
            // months trigger = ON CHANGE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE)
            {
                // set the register and turn on the interrupt
                rtc_reg->months_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_CHANGE;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_MONTHS;
            }
            // months trigger = ON VALUE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
            {
                // set the register (include the value) and turn on the interrupt
                // convert the value to BCD first]
                uint32_t bcd_trigger_value = convert_int_to_bcd(trigger_value);
                rtc_reg->months_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_VALUE | bcd_trigger_value;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_MONTHS;
            }
            break;

        // *** years ***
        case TR_HAL_RTC_TIME_UNIT_YEARS:

            // check range if we are setting a value
            if (   (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
                && (  (trigger_value > TR_HAL_RTC_YEAR_MAXIMUM)
                  ||  (trigger_value < TR_HAL_RTC_YEAR_MINIMUM)))
            {
                return TR_HAL_RTC_INVALID_YEAR;
            }
            // years trigger = OFF
            if (event_trigger == TR_HAL_EVENT_TRIGGER_OFF)
            {
                // set register and turn off interrupt
                rtc_reg->years_event_trigger = TR_HAL_REG_SETTING_EVENT_OFF;
                rtc_reg->interrupt_enable &= (~TR_HAL_RTC_INTERRUPT_YEARS);
            }
            // years trigger = ON CHANGE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_UNIT_CHANGE)
            {
                // set the register and turn on the interrupt
                rtc_reg->years_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_CHANGE;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_YEARS;
            }
            // years trigger = ON VALUE
            else if (event_trigger == TR_HAL_EVENT_TRIGGER_ON_SPECIFIC_VALUE)
            {
                // set the register (include the value) and turn on the interrupt
                // convert the value to BCD first]
                uint32_t bcd_trigger_value = convert_int_to_bcd(trigger_value);
                rtc_reg->years_event_trigger = TR_HAL_REG_SETTING_EVENT_ON_VALUE | bcd_trigger_value;
                rtc_reg->interrupt_enable |= TR_HAL_RTC_INTERRUPT_YEARS;
            }
            break;

        // *** invalid and default (unhandled) ***
        case TR_HAL_RTC_TIME_UNIT_INVALID:
        default:
            return TR_HAL_RTC_INVALID_TIME_UNIT;
    }

    // this tells the chip we are done updating the event trigger registers
    // and to start using the new ones
    if (g_rtc_ok_to_run_load)
    {
        rtc_reg->load = TR_HAL_RTC_UPDATE_EVENT_TRIGGER;
    }
    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_rtc_set_combo_trigger_event
///
/// combo event (can only have 1 of these at a time - chip restriction)
/// if a combo event is set ALL other time unit events are canceled (turned off)
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_set_combo_trigger_event(tr_hal_rtc_date_time new_date_time)
{
    // NOTE: if there are any time unit events setup, they will be turned off
    // this is done in two parts: the event register for all time units is
    // set at the end of this function, and ALL interrupts OTHER THAN the
    // combo event interrupt are disabled.

    // if there is ANOTHER combo event active, it will be overwritten with
    // this event

    // chip register address
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // the event can include multiple time units
    // it MUST include seconds and minutes (or else the other event types can be used)
    // once it stops including a time unit, it can't include any larger time units
    // so first, figure out which time unit the event goes up to
    uint16_t last_time_unit_to_use = TR_HAL_RTC_TIME_UNIT_INVALID;

    // check seconds
    if ((new_date_time.time.seconds == TR_HAL_INVALID_DATE_TIME_VALUE)
        || (new_date_time.time.seconds > TR_HAL_RTC_SECOND_MAXIMUM))
    {
        return TR_HAL_RTC_INVALID_SECOND;
    }

    // check minutes
    if ((new_date_time.time.minutes == TR_HAL_INVALID_DATE_TIME_VALUE)
        || (new_date_time.time.minutes > TR_HAL_RTC_MINUTE_MAXIMUM))
    {
        return TR_HAL_RTC_INVALID_MINUTE;
    }

    // if hours is set for invalid, we stop at minutes
    if (new_date_time.time.hours == TR_HAL_INVALID_DATE_TIME_VALUE)
    {
        last_time_unit_to_use = TR_HAL_RTC_TIME_UNIT_MINUTES;
    }
    // if hours is not invalid, keep going
    else
    {
        // this is when the caller tries to pass an hours value
        if (new_date_time.time.hours > TR_HAL_RTC_HOUR_MAXIMUM)
        {
            return TR_HAL_RTC_INVALID_HOUR;
        }

        // if days is set for invalid, we stop at hours
        if (new_date_time.date.days == TR_HAL_INVALID_DATE_TIME_VALUE)
        {
            last_time_unit_to_use = TR_HAL_RTC_TIME_UNIT_HOURS;
        }
        // if days is not invalid, keep going
        else
        {
            // this is when the caller tries to pass a days value
            if ((new_date_time.date.days < TR_HAL_RTC_DAY_MINIMUM)
                || (new_date_time.date.days > TR_HAL_RTC_DAY_MAXIMUM))
            {
                return TR_HAL_RTC_INVALID_DAY;
            }

            // if months is set for invalid, we stop at days
            if (new_date_time.date.months == TR_HAL_INVALID_DATE_TIME_VALUE)
            {
                last_time_unit_to_use = TR_HAL_RTC_TIME_UNIT_DAYS;
            }
            else
            {
                // this is when the caller tries to pass a months value
                if ((new_date_time.date.months < TR_HAL_RTC_MONTH_MINIMUM)
                    || (new_date_time.date.months > TR_HAL_RTC_MONTH_MAXIMUM))
                {
                    return TR_HAL_RTC_INVALID_MONTH;
                }

                // if years is set for invalid, we stop at months
                if (new_date_time.date.years == TR_HAL_INVALID_DATE_TIME_VALUE)
                {
                    last_time_unit_to_use = TR_HAL_RTC_TIME_UNIT_MONTHS;
                }
                else
                {
                    // this is when the caller tries to pass a years value
                    if ((new_date_time.date.years < TR_HAL_RTC_YEAR_MINIMUM)
                        || (new_date_time.date.years > TR_HAL_RTC_YEAR_MAXIMUM))
                    {
                        return TR_HAL_RTC_INVALID_YEAR;
                    }
                    last_time_unit_to_use = TR_HAL_RTC_TIME_UNIT_YEARS;
                }

            }
        }
    }

    // in order to enable the combo event, we need to set the
    // combo event flag in the register that is the time unit
    // AFTER the one it ends on
    // for instance: if ends on MINS then set it on HOURS

    // seconds and minutes are always set
    rtc_reg->seconds_event_trigger = convert_int_to_bcd(new_date_time.time.seconds);
    rtc_reg->minutes_event_trigger = convert_int_to_bcd(new_date_time.time.minutes);

    // if we end on minutes
    if (last_time_unit_to_use == TR_HAL_RTC_TIME_UNIT_MINUTES)
    {
        rtc_reg->hours_event_trigger = TR_HAL_REG_SETTING_COMBO_EVENT;
        // clear registers not in use
        rtc_reg->days_event_trigger = 0;
        rtc_reg->months_event_trigger = 0;
        rtc_reg->years_event_trigger = 0;
    }

    // if we end on hours
    else if (last_time_unit_to_use == TR_HAL_RTC_TIME_UNIT_HOURS)
    {
        rtc_reg->hours_event_trigger = convert_int_to_bcd(new_date_time.time.hours);
        rtc_reg->days_event_trigger = TR_HAL_REG_SETTING_COMBO_EVENT;
        // clear registers not in use
        rtc_reg->months_event_trigger = 0;
        rtc_reg->years_event_trigger = 0;
    }

    // if we end on days
    else if (last_time_unit_to_use == TR_HAL_RTC_TIME_UNIT_DAYS)
    {
        rtc_reg->hours_event_trigger = convert_int_to_bcd(new_date_time.time.hours);
        rtc_reg->days_event_trigger = convert_int_to_bcd(new_date_time.date.days);
        rtc_reg->months_event_trigger = TR_HAL_REG_SETTING_COMBO_EVENT;
        // clear registers not in use
        rtc_reg->years_event_trigger = 0;
    }

    // if we end on months
    else if (last_time_unit_to_use == TR_HAL_RTC_TIME_UNIT_MONTHS)
    {
        rtc_reg->hours_event_trigger = convert_int_to_bcd(new_date_time.time.hours);
        rtc_reg->days_event_trigger = convert_int_to_bcd(new_date_time.date.days);
        rtc_reg->months_event_trigger = convert_int_to_bcd(new_date_time.date.months);
        rtc_reg->years_event_trigger = TR_HAL_REG_SETTING_COMBO_EVENT;
    }

    // if we end on years
    else if (last_time_unit_to_use == TR_HAL_RTC_TIME_UNIT_YEARS)
    {
        rtc_reg->hours_event_trigger = convert_int_to_bcd(new_date_time.time.hours);
        rtc_reg->days_event_trigger = convert_int_to_bcd(new_date_time.date.days);
        rtc_reg->months_event_trigger = convert_int_to_bcd(new_date_time.date.months);
        rtc_reg->years_event_trigger = convert_int_to_bcd( (new_date_time.date.years) - 2000 );
        // when YEARS is included, we leave the bits CLEAR
        // see data sheet 9.4.14 if you don't believe me
        //rtc_reg->years_event_trigger = TR_HAL_REG_SETTING_COMBO_EVENT;
    }

    // keep track that we have a combo event enabled
    g_combo_event_active = true;

    // if the combo event is enabled we need to TURN OFF the other interrupts.
    // so instead of reading the interrupt enable register, we just want them all to be OFF
    //uint32_t current_interrupt_settings = rtc_reg->interrupt_enable;
    //uint32_t new_interrupt_settings = current_interrupt_settings |= TR_HAL_RTC_INTERRUPT_COMBINED_EVENT;

    // only enable the combo event interrupt
    rtc_reg->interrupt_enable = TR_HAL_RTC_INTERRUPT_COMBINED_EVENT;

    // this tells the chip we are done updating the event trigger registers
    // and to start using the new one
    if (g_rtc_ok_to_run_load)
    {
        rtc_reg->load = TR_HAL_RTC_UPDATE_EVENT_TRIGGER;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// interrupt handler
/// ***************************************************************************
void rtc_handler(void)
{
    // chip register address
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // the bitmask to return
    uint32_t event_bitmask = 0;

    // read the interrupt(s)
    uint32_t interrupt_triggered_mask = rtc_reg->interrupt_status;

    // seconds
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_SECONDS)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_SECONDS;
    }
    // minutes
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_MINUTES)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_MINUTES;
    }
    // hours
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_HOURS)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_HOURS;
    }
    // days
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_DAYS)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_DAYS;
    }
    // months
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_MONTHS)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_MONTHS;
    }
    // years
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_YEARS)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_YEARS;
    }
    // combined event
    if (interrupt_triggered_mask & TR_HAL_RTC_INTERRUPT_COMBINED_EVENT)
    {
        event_bitmask |= TR_HAL_RTC_EVENT_TRIGGERED_COMBINED_EVENT;
    }

    // clear interrupt state
    rtc_reg->interrupt_clear = interrupt_triggered_mask;

    // send the event(s) back to the user
    if (g_rtc_callback != NULL)
    {
        // get the current date and time
        tr_hal_rtc_date_time date_time_now;
        tr_hal_rtc_get_date_time(&date_time_now);

        // call the user function and pass the events and the current date time
        g_rtc_callback(event_bitmask, date_time_now);
    }
}

/// ***************************************************************************
/// tr_hal_rtc_speedup_for_testing
///
/// this causes the time unit specified to speed up to change once per second
/// ***************************************************************************
tr_hal_status_t tr_hal_rtc_speedup_for_testing(tr_hal_rtc_speedup_unit_t speedup_unit)
{
    // chip register address
    RTC_REGISTERS_T* rtc_reg = tr_hal_rtc_get_register_address();

    // set the desired speedup unit
    rtc_reg->control = speedup_unit;

    return TR_HAL_SUCCESS;
}


