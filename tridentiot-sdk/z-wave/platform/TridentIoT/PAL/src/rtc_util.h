/// ***************************************************************************
///
/// @file rtc_util.h
///
/// @brief The file contains the function declarations for the RTC timer
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef RTC_UTIL_H_
#define RTC_UTIL_H_
#include "stdint.h"


void setup_rtc_alarm(uint32_t time_in_ms);
uint32_t get_rtc_time(void);
uint32_t get_rtc_duration(uint32_t current_time);
#endif /* RTC_UTIL_H_ */
