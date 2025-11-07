// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_timer.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Zwave Timer module.
 */

/* Z-Wave includes */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <ZW_timer.h>
#include <SwTimerLiaison.h>
#include "SizeOf.h"

/**
* ZwTimer object
*/
typedef struct  SZwTimer
{
  SSwTimerLiaison TimerLiaison;     /**<  TimerLiaison object for ZWave task */
  SSwTimer* aTimerPointerArray[32]; /**<  Array for TimerLiaison - for keeping registered timers */
} SZwTimer;


/**
* ZwTimer is a singleton, thus containing own object.
*/
static SZwTimer g_ZwTimer = { 0 };


void ZwTimerInit(uint8_t iTaskNotificationBitNumber, void *ReceiverTask)
{
  TimerLiaisonInit(
                    &g_ZwTimer.TimerLiaison,
                    sizeof_array(g_ZwTimer.aTimerPointerArray),
                    g_ZwTimer.aTimerPointerArray,
                    iTaskNotificationBitNumber,
                    (TaskHandle_t)ReceiverTask
                  );
}


bool ZwTimerRegister(
                SSwTimer* pTimer,
                bool bAutoReload,
                void(*pCallback)(SSwTimer* pTimer)
              )
{
  ESwTimerLiaisonStatus eStatus;

  eStatus = TimerLiaisonRegister(
                              &g_ZwTimer.TimerLiaison,
                              pTimer,
                              bAutoReload,
                              pCallback
                            );
  return eStatus == ESWTIMERLIAISON_STATUS_SUCCESS ? true : false;
}


void ZwTimerNotificationHandler(void)
{
  TimerLiaisonNotificationHandler(&g_ZwTimer.TimerLiaison);
}


void ZwTimerSetReceiverTask(void *ReceiverTask)
{
  TimerLiaisonSetReceiverTask(&g_ZwTimer.TimerLiaison, (TaskHandle_t)ReceiverTask);
}


void ZwTimerStopAll(void)
{
  /*Stops all timers */
  for (uint32_t i = 0; i < g_ZwTimer.TimerLiaison.iTimerCount; i ++)
  {
    TimerStop(g_ZwTimer.aTimerPointerArray[i]);
  }
}
