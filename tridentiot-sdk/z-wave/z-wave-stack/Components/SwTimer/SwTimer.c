// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* Software timer module.
*
* @copyright 2018 Silicon Laboratories Inc.
*/

/* Z-Wave includes */
#include <SwTimerPrivate.h>
#include <SwTimerLiaison.h>

/* FreeeRTOS includes*/
#include <FreeRTOS.h>
#include <projdefs.h>
#include <timers.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

#include "Assert.h"


STATIC_ASSERT(sizeof(((struct SSwTimer *)0)->dummy) == sizeof(SSwTimerPrivate_t), STATIC_ASSERT_FAILED_SwTimer_types_does_not_match_in_size);

void TimerSetCallback(SSwTimer* pTimer, void(*pCallback)(SSwTimer* pTimer))
{
  ASSERT(pTimer);

  pTimer->pCallback = pCallback;
}


ESwTimerStatus TimerStart(SSwTimer* pTimer, uint32_t iTimeout)
{
  ASSERT(pTimer);
  
  BaseType_t Status;

  /* Make sure timer has been created before using it */
  if ( NULL == pTimer->TimerHandle )
  {
     return ESWTIMER_STATUS_FAILED;
  }

  /*
   * Suspend the task scheduler while calling the xTimer API functions below. This is
   * to avoid switching back and forth between the calling task and the SW Timer task
   * (which has higher priority) for each event put on the timer event queue.
   */
  vTaskSuspendAll();

  Status = xTimerStop(pTimer->TimerHandle, 0);
  if (pdPASS == Status)
  {
    /* Change the timer periode */
    Status = xTimerChangePeriod(pTimer->TimerHandle, pdMS_TO_TICKS(iTimeout), 0);

    if (pdPASS == Status)
    {
      TimerLiaisonClearPendingTimerEvent(pTimer->pLiaison, pTimer->Id);

      /* Start the timer */
      Status = xTimerStart(pTimer->TimerHandle, 0);
    }
  }

  xTaskResumeAll();
  return Status == pdPASS ? ESWTIMER_STATUS_SUCCESS : ESWTIMER_STATUS_FAILED;
}


ESwTimerStatus TimerStartFromISR(SSwTimer* pTimer, uint32_t iTimeout)
{
  ASSERT(pTimer);

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t Status;

  /* Make sure timer has been created before using it */
  if ( NULL == pTimer->TimerHandle )
  {
     return ESWTIMER_STATUS_FAILED;
  }

  Status = xTimerStopFromISR(pTimer->TimerHandle, &xHigherPriorityTaskWoken);
  if (pdPASS == Status)
  {
    /* Change the timer periode */
    Status = xTimerChangePeriodFromISR(pTimer->TimerHandle, pdMS_TO_TICKS(iTimeout), &xHigherPriorityTaskWoken);

    if (pdPASS == Status)
    {
      TimerLiaisonClearPendingTimerEventFromISR(pTimer->pLiaison, pTimer->Id);

      /* Start the timer */
      Status = xTimerStartFromISR(pTimer->TimerHandle, &xHigherPriorityTaskWoken);
    }
  }

  /*
   * Events have been added to the SW Timer event queue.
   * We want these events to be processed as fast as possible to avoid queue
   * overflow. Therefore tell the scheduler to switch to the SW Timer task
   * immediately after returning from the interrupt handler.
   */
#ifdef __arm__
  /* This assembler macro doesn't compile for Unit Test on Windows. Hence #ifdef __arm__ */
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

  return Status == pdPASS ? ESWTIMER_STATUS_SUCCESS : ESWTIMER_STATUS_FAILED;
}


ESwTimerStatus TimerRestart(SSwTimer* pTimer)
{
  ASSERT(pTimer);
  BaseType_t Status;

  TimerLiaisonClearPendingTimerEvent(pTimer->pLiaison, pTimer->Id);
  Status = xTimerReset(pTimer->TimerHandle, 0);
  return Status == pdPASS ? ESWTIMER_STATUS_SUCCESS : ESWTIMER_STATUS_FAILED;
}


ESwTimerStatus TimerRestartFromISR(SSwTimer* pTimer)
{
  ASSERT(pTimer);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t Status;

  TimerLiaisonClearPendingTimerEventFromISR(pTimer->pLiaison, pTimer->Id);
  Status = xTimerResetFromISR(pTimer->TimerHandle, &xHigherPriorityTaskWoken);

  /*
   * Events have been added to the SW Timer event queue.
   * We want these events to be processed as fast as possible to avoid queue
   * overflow. Therefore tell the scheduler to switch to the SW Timer task
   * immediately after returning from the interrupt handler.
   */
#ifdef __arm__
  /* This assembler macro doesn't compile for Unit Test on Windows. Hence #ifdef __arm__ */
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
  return Status == pdPASS ? ESWTIMER_STATUS_SUCCESS : ESWTIMER_STATUS_FAILED;
}


ESwTimerStatus TimerStop(SSwTimer* pTimer)
{
  ASSERT(pTimer);
  BaseType_t Status;

  /* Make sure timer has been created before using it */
  if ( NULL == pTimer->TimerHandle )
  {
     return ESWTIMER_STATUS_FAILED;
  }

  Status = xTimerStop(pTimer->TimerHandle, 0);
  if (pdFAIL != Status)
  {
    TimerLiaisonClearPendingTimerEvent(pTimer->pLiaison, pTimer->Id);
  }
  
  return Status == pdPASS ? ESWTIMER_STATUS_SUCCESS : ESWTIMER_STATUS_FAILED;
}


ESwTimerStatus TimerStopFromISR(SSwTimer* pTimer)
{
  ASSERT(pTimer);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t Status;

  /* Make sure timer has been created before using it */
  if ( NULL == pTimer->TimerHandle )
  {
     return ESWTIMER_STATUS_FAILED;
  }

  Status = xTimerStopFromISR(pTimer->TimerHandle, &xHigherPriorityTaskWoken);
  if (pdFAIL != Status)
  {
    TimerLiaisonClearPendingTimerEventFromISR(pTimer->pLiaison, pTimer->Id);
  }

  /*
   * Events have been added to the SW Timer event queue.
   * We want these events to be processed as fast as possible to avoid queue
   * overflow. Therefore tell the scheduler to switch to the SW Timer task
   * immediately after returning from the interrupt handler.
   */
#ifdef __arm__
  /* This assembler macro doesn't compile for Unit Test on Windows. Hence #ifdef __arm__ */
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

  return Status == pdPASS ? ESWTIMER_STATUS_SUCCESS : ESWTIMER_STATUS_FAILED;
}


bool TimerIsActive(SSwTimer* pTimer)
{
  ASSERT(pTimer);

  /* Make sure timer has been created before using it */
  if ( NULL == pTimer->TimerHandle )
  {
     return false;
  }

  BaseType_t Status = xTimerIsTimerActive(pTimer->TimerHandle);

  return ((TimerHasPendingCallback(pTimer)) || (Status == pdTRUE));
}

bool TimerHasPendingCallback(SSwTimer* pTimer)
{
  return TimerLiaisonHasPendingTimerEvent(pTimer->pLiaison, pTimer->Id);
}

bool TimerHasPendingCallbackFromISR(SSwTimer* pTimer)
{
  return TimerLiaisonHasPendingTimerEventFromISR(pTimer->pLiaison, pTimer->Id);
}

ESwTimerStatus TimerGetMsUntilTimeout(SSwTimer* pTimer, uint32_t refTaskTickCount, uint32_t *pMsUntilTimeout)
{
  ASSERT(pTimer);
  ASSERT(pMsUntilTimeout);

  /* Make sure timer has been created before using it */
  if ( NULL == pTimer->TimerHandle )
  {
     return ESWTIMER_STATUS_FAILED;
  }

  if (xTimerIsTimerActive(pTimer->TimerHandle) != pdFALSE)
  {
    TickType_t expiryTime = xTimerGetExpiryTime(pTimer->TimerHandle);

    /* We assume one tick is equal to one millisecond
     * (FreeRTOS configTICK_RATE_HZ = 1000)
     */
    if (expiryTime >= refTaskTickCount)
    {
      *pMsUntilTimeout = expiryTime - refTaskTickCount;
    }
    else
    {
      /* The 32-bit expiry timer has wrapped around */
      *pMsUntilTimeout = (UINT32_MAX - refTaskTickCount) + expiryTime;
    }

    return ESWTIMER_STATUS_SUCCESS;
  }
  return ESWTIMER_STATUS_FAILED;
}

uint32_t TimerGetPeriod(SSwTimer *pTimer)
{
  ASSERT(pTimer);
  return xTimerGetPeriod(pTimer->TimerHandle);
}

