/// ***************************************************************************
///
/// @file zpal_power_manager_private.h
///
/// @brief Platform specific extension of ZPAL Power Manager API
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_POWER_MANAGER_PRIVATE_H_
#define ZPAL_POWER_MANAGER_PRIVATE_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <FreeRTOS.h>

/**
 * @addtogroup zpal
 * @brief
 * Z-Wave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal-power-manager
 * @brief
 * Platform specific extension of ZPAL Power Manager API
 *
 * @{
 */

void zpal_pm_enter_sleep(TickType_t sleep_ticks);

void zpal_pm_exit_sleep(void);

/**
 * @} //zpal-power-manager
 * @} //zpal
 */

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_POWER_MANAGER_PRIVATE_H_ */
