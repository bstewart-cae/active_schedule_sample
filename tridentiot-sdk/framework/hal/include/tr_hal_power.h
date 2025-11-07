/// ****************************************************************************
/// @file tr_hal_rtc.h
///
/// @brief This is the common include file for the Trident HAL Power Mgmt
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_POWER_H_
#define TR_HAL_POWER_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_power Power Mgmt
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************

// this is to set the power mode to wake, lite sleep, deep sleep, power down
// this calls all the HAL modules so they are in the correct state
// not yet implemented
//tr_hal_status_t tr_hal_power_set_power_mode(tr_hal_power_mode_t power_mode);

// we have some specific functions for enabling, disabling, and reading enabled
// state of clocks
tr_hal_status_t tr_hal_power_enable_clock(tr_hal_clock_t clock);
tr_hal_status_t tr_hal_power_disable_clock(tr_hal_clock_t clock);
tr_hal_status_t tr_hal_power_is_clock_enabled(tr_hal_clock_t clock,
                                              bool* is_enabled);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_POWER_H_
