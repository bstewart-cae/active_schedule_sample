/// ****************************************************************************
/// @file tr_hal_rtc.h
///
/// @brief This is the common include file for the Trident HAL RTC Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_RTC_H_
#define TR_HAL_RTC_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// \brief RTC Driver API functions
/// ****************************************************************************

/// ****************************************************************************
/// @defgroup tr_gpio_rtc RTC
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************

/// ***************************************************************************
/// ---- RTC init ----
/// ***************************************************************************

// this can be used to set the time and date, in addition to event and
// interrupt settings. Unlike other HAL modules, no uninit function is
// needed, and init can be called over and over, and just resets the
// RTC to the settings in the struct passed in
tr_hal_status_t tr_hal_rtc_init(tr_hal_rtc_settings_t* rtc_settings);

// see if the RTC is running
// this allows apps to initialzie it if not
tr_hal_status_t tr_hal_rtc_is_running(bool* is_enabled);


/// ***************************************************************************
/// ---- read time and date ----
/// ***************************************************************************

// read time
tr_hal_status_t tr_hal_rtc_get_time(tr_hal_rtc_time* return_time);

// read date
tr_hal_status_t tr_hal_rtc_get_date(tr_hal_rtc_date* return_date);

// read time and date
tr_hal_status_t tr_hal_rtc_get_date_time(tr_hal_rtc_date_time* return_date_time);

// check the difference between 2 time structs and if that diff is
// greater than the check_against_time passed in
bool tr_hal_rtc_diff_greater_than_ms(tr_hal_rtc_time* time1,
                                     tr_hal_rtc_time* time2,
                                     uint32_t check_against_ms);

/// ***************************************************************************
/// ---- set time and date ----
/// ***************************************************************************

// do not need these if use the settings struct and the init function

// set RTC time
tr_hal_status_t tr_hal_rtc_set_time(tr_hal_rtc_time new_time);

// set RTC date
tr_hal_status_t tr_hal_rtc_set_date(tr_hal_rtc_date new_date);

// set RTC time and date
tr_hal_status_t tr_hal_rtc_set_date_time(tr_hal_rtc_date_time new_date_time);


/// ***************************************************************************
/// ---- set event handler ----
/// ***************************************************************************

// do not need this if use the settings struct and the init function

// set RTC event handler
tr_hal_status_t tr_hal_rtc_set_event_handler(tr_hal_rtc_event_callback_t new_event_handler);


/// ***************************************************************************
/// ---- interrupt handling ----
/// ***************************************************************************

// do not need these if use the settings struct and the init function

// set RTC interrupt handling
tr_hal_status_t tr_hal_rtc_set_interrupt_behavior(bool             enable_chip_interrupts,
                                                  tr_hal_int_pri_t interrupt_priority,
                                                  bool             wake_on_interrupt);

tr_hal_status_t tr_hal_rtc_get_interrupt_behavior(bool*             enable_chip_interrupts,
                                                  tr_hal_int_pri_t* interrupt_priority,
                                                  bool*             wake_on_interrupt);

/// ***************************************************************************
/// ---- clock divisor ----
/// ***************************************************************************
// do not need these if use the settings struct and the init function

// read clock divisor
tr_hal_status_t tr_hal_rtc_get_clock_divisor(uint32_t* clock_divisor);

// set clock divisor
tr_hal_status_t tr_hal_rtc_set_clock_divisor(uint32_t clock_divisor);


/// ***************************************************************************
/// ---- events (interrupts) ----
///
/// there are multiple ways to set an event trigger:
///
/// 1. no event
/// 2. event on every unit change (ex: every hour or every day, right after unit change)
/// 3. event on specific unit value (ex: when hour==6 or when day==12
/// 4. combo event that uses multiple time unit settings (ex: every hour==12, min==33, sec==12)
///
/// each time unit (year, month, day, hour, minute, second) can set one event  for
/// either #2 or #3, OR the SYSTEM can set one combo event (#4)
///
/// The combo event (#4) does not need to use all time units, but if it does not
/// use one time unit it cannot use any time units greater than the time unit not used
/// ***************************************************************************

// to setup an event of type #2 (ON_CHANGE) or type #3 (ON_VALUE)
tr_hal_status_t tr_hal_rtc_set_time_trigger_event(tr_hal_rtc_time_unit_t     time_unit,
                                                  tr_hal_rtc_event_trigger_t event_trigger,
                                                  uint16_t                   trigger_value);

// to setup combo event (type #4)
// can only have 1 of these at a time - chip restriction
tr_hal_status_t tr_hal_rtc_set_combo_trigger_event(tr_hal_rtc_date_time new_date_time);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_RTC_H_
