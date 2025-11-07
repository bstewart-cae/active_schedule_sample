// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file SwTimerPrivate.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * Offers a private API to the software timer functionality.
 *
 * This API must be used only by the stack and in relation to testing.
 *
 * This header file must not be shipped to customers.
 */
#include <SwTimer.h>
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>
#include <stdint.h>

#ifndef COMPONENTS_SWTIMER_SWTIMERPRIVATE_H_
#define COMPONENTS_SWTIMER_SWTIMERPRIVATE_H_

/**
* @addtogroup Components
* @{
* @addtogroup Timer
* @{
* @addtogroup SwTimer
* @{
*/

/**
* Timer Liaison object. All content is private.
*/
typedef struct SSwTimerPrivate_t
{
  StaticTimer_t xTimer;                 /**< Static buffer to store FreeRTOS timer */
} SSwTimerPrivate_t;

/**
 * @} // addtogroup SwTimer
 * @} // addtogroup Timer
 * @} // addtogroup Components
 */

#endif /* COMPONENTS_SWTIMER_SWTIMERPRIVATE_H_ */
