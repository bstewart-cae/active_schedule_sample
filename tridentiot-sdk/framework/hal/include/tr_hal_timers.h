/// ****************************************************************************
/// @file tr_hal_timers.h
///
/// @brief This is the common include file for the Trident HAL Timer Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_TIMERS_H_
#define TR_HAL_TIMERS_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// \brief Timer Driver API functions
/// ****************************************************************************


/// ****************************************************************************
/// @defgroup tr_hal_timers Timers
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


/// ****************************************************************************
/// --- init settings, read settings ----
/// ****************************************************************************

// init timer
tr_hal_status_t tr_hal_timer_init(tr_hal_timer_id_t        timer_id,
                                  tr_hal_timer_settings_t* timer_settings);

// read timer info
// this loads the current timer info into the timer_settings passed in
tr_hal_status_t tr_hal_timer_read(tr_hal_timer_id_t        timer_id,
                                  tr_hal_timer_settings_t* timer_settings);



/// ****************************************************************************
/// --- start, stop, restart ----
/// ****************************************************************************

// start timer
tr_hal_status_t tr_hal_timer_start(tr_hal_timer_id_t timer_id);

// stop timer
tr_hal_status_t tr_hal_timer_stop(tr_hal_timer_id_t timer_id);

// restart
tr_hal_status_t tr_hal_timer_restart(tr_hal_timer_id_t timer_id);

// is timer running?
tr_hal_status_t tr_hal_timer_is_running(tr_hal_timer_id_t timer_id,
                                        bool* is_running);


/// ****************************************************************************
/// ---- read active timer, change timer period, set callback fx ----
/// ****************************************************************************

// read current state of active timer
// if status=TR_HAL_SUCCESS and timer_enabled is set to TRUE then the other 2 params are valid
tr_hal_status_t tr_hal_read_active_timer_state(tr_hal_timer_id_t         timer_id,
                                               bool*                     timer_enabled,
                                               tr_hal_timer_prescalar_t* prescalar,
                                               uint32_t*                 current_countdown_value);

// change period - we only allow editing of the period. If the
// callback function changes it needs to be a new timer
tr_hal_status_t tr_hal_timer_change_period(tr_hal_timer_id_t        timer_id,
                                           uint32_t                 timer_start_value,
                                           tr_hal_timer_prescalar_t prescalar);

// set just the callback
tr_hal_status_t tr_hal_timer_set_callback_function(tr_hal_timer_id_t       timer_id,
                                                   tr_hal_timer_callback_t callback_function);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_TIMERS_H_
