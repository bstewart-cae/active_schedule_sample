// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_timer.h
 * Zwave Timer module - Ensures timer callbacks are performed in the ZWave task.
 * The module wraps a SwTimerLiaison and presents it as a "singleton", easing its
 * availability among the Zwave protocol modules.
 *
 * The modules basically wraps a SwTimerLiaison, owning the required objects
 * as static data.
 *
 * The module is only intended for use inside the protocol.
 *
 * The iTaskNotificationBitNumber (given at ZwTimerInit) bit will be set as
 * TaskNotification to the ReceiverTask (given at ZwTimerInit) when a registered
 * timer expires. When such a notification is set, the notification bit must be
 * cleared and ZwTimerNotificationHandler called.
 * 
 * For detailed documentation see SwTimerLiaison.h
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */

#ifndef _ZW_TIMER_H_
#define _ZW_TIMER_H_

#include <stdbool.h>
#include <SwTimer.h>


/**
* Initialize ZwTimer
*
* @param[in]     iTaskNotificationBitNumber Number defines which bit to use when notifying
*                                           receiver task of pending timer event (range 0 - 31)
* @param[in]     ReceiverTask               Handle to the Zwave task
*/
void ZwTimerInit(uint8_t iTaskNotificationBitNumber, void *ReceiverTask);

/**
* Configures the Receiver task of callbacks.
*
* Provided as the ZwTimer may be needed for registrering SwTimers before
* the receiver task is created, meaning ZwTimerInit is called prior to
* the Receiver task handle being available.
* Though not the intention, the receiver task can be changed run time.
*
* @param[in]     ReceiverTask             Handle to the Zwave task
*/
void ZwTimerSetReceiverTask(void *ReceiverTask);


/**
* Register a SwTimer to a TimerLiaison.
* Initial SwTimer configuration is also performed.
*
* Method creates a static FreeRTOS timer from the SwTimer object passed as argument,
* and registers it as requiring callbacks in the Receiver task.
* If calling method ZwTimerRegister on the same SwTimer object multiple times,
* all but the first call will be ignored.
* @param[in]    pTimer        Pointer to the SwTimer object to register
* @param[in]    bAutoReload   Enable timer auto reload on timeout. Configuration
*                             of AutoReload cannot be changed after registration.
* @param[in]    pCallback     Callback method of type void Callback(SSwTimer* pTimer).
*                             Argument may be NULL, in which case no callback is performed.
* @retval       true          Timer successfully registered to ZwTimer TimerLiaison
* @retval       false         ZwTimer is full and cannot register any more SwTimers.
*/
bool ZwTimerRegister(
  SSwTimer* pTimer,
  bool bAutoReload,
  void(*pCallback)(SSwTimer* pTimer)
  );


/**
* Must be called from Zwave task when the task notification bit
* assigned to ZwTimer is set.
*
* Method will perform pending callbacks. There is no side effects from calling
* method when assigned task notification bit was not set.
*/
void ZwTimerNotificationHandler(void);

/**
* Stop all created protocol Timers
*
* Method will clear pending callbacks.
*/
void ZwTimerStopAll(void);

#endif /* _ZW_TIMER_H_ */
