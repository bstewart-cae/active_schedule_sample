/// ****************************************************************************
/// @file tr_hal_wdog.h
///
/// @brief This is the common include file for the Trident HAL Watchdog Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_WDOG_H_
#define TR_HAL_WDOG_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// \brief Watchdog API functions
/// ****************************************************************************


/// ****************************************************************************
/// @defgroup tr_hal_wdog Watchdog Timer
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


/// ***************************************************************************
/// ---- watchdog init ----
/// ***************************************************************************

// loads the WDOG chip registers with the settings passed in
// returns an error if settings are incompatible
tr_hal_status_t tr_hal_wdog_init(tr_hal_wdog_settings_t* wdog_settings);


/// ***************************************************************************
/// ---- watchdog enable / disable / enabled status ----
/// ***************************************************************************

// enable / disable watchdog
tr_hal_status_t tr_hal_wdog_enable(void);
tr_hal_status_t tr_hal_wdog_disable(void);

// is watchdog currently running
tr_hal_status_t tr_hal_is_wdog_enabled(bool* is_enabled);


/// ***************************************************************************
/// ---- watchdog reset ----
/// ***************************************************************************

// starts timer over again from initial value
tr_hal_status_t tr_hal_wdog_reset(void);


/// ***************************************************************************
/// ---- get watchdog status ----
/// ***************************************************************************

// read how many resets have been due to the WDOG
tr_hal_status_t tr_hal_wdog_read_num_resets(uint32_t* num_resets);

// read the current state of the watchdog from register values (where possible)
tr_hal_status_t tr_hal_wdog_read_curr_state(tr_hal_wdog_settings_t* wdog_settings);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_WDOG_H_
