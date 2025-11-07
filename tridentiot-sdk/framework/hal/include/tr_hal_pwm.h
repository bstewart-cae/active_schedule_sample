/// ****************************************************************************
/// @file tr_hal_pwm.h
///
/// @brief This is the common include file for the Trident HAL PWM Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_PWM_H_
#define TR_HAL_PWM_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// \brief PWM Driver API functions
/// ****************************************************************************


/// ****************************************************************************
/// @defgroup tr_hal_pwm PWM
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


/// ****************************************************************************
/// ---- init settings / read settings ----
/// ****************************************************************************

// error check and then load the settings into the registers
tr_hal_status_t tr_hal_pwm_init(tr_hal_pwm_id_t        pwm_id,
                                tr_hal_pwm_settings_t* pwm_settings);

// sets GPIO back to general GPIO and turns off interrupts, etc
tr_hal_status_t tr_hal_pwm_uninit(tr_hal_pwm_id_t pwm_id);


// this loads the current PWM settings into the pwm_settings passed in
tr_hal_status_t tr_hal_pwm_settings_read(tr_hal_pwm_id_t        pwm_id,
                                         tr_hal_pwm_settings_t* pwm_settings);


/// ****************************************************************************
/// ---- start / stop / is_running ----
/// ****************************************************************************

tr_hal_status_t tr_hal_pwm_start(tr_hal_pwm_id_t pwm_id);

tr_hal_status_t tr_hal_pwm_stop(tr_hal_pwm_id_t pwm_id);

tr_hal_status_t tr_hal_pwm_is_running(tr_hal_pwm_id_t pwm_id,
                                      bool*           is_running);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_PWM_H_

