/**
 * @file app_hw.h
 * Platform abstraction for Z-Wave Applications
 *
 * @copyright 2023 Silicon Laboratories Inc.
 */
/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef APP_HW_H_
#define APP_HW_H_

#include <stdint.h>

/**
 * Initialize hardware modules specific to a given z-wave application.
 *
 * @note Each application/port combination MUST implement this function
 */
extern void app_hw_init(void);

/**
 * Deep sleep wakeup handler. Called after application wakes up from deep sleep.
 *
 * @note Each application/port combination CAN implement this function
 */
void app_hw_deep_sleep_wakeup_handler(void);

/**
 * This function initializes any HW component that must be ready before the Z-Wave stack starts up.
 *
 * @note This is a WEAK function.
 *       If the application doesn't require any HW component to be initialize before the Z-Wave stack starts up,
 *       then it can be omitted
*/
void  zwsdk_app_hw_init(void);
#endif /* APP_HW_H_ */
