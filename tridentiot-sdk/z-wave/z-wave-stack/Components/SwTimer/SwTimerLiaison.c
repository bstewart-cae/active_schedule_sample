// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* Software timer liaison module.
*
* @copyright 2018 Silicon Laboratories Inc.
*
* @startuml
* title SwTimer / SwTimerLiaison  usage sequence
* participant TimerUser
* participant SwTimerLiaison
* participant SwTimer
* participant FreeRTOS_Timer
* participant ReceiverTask
* == Initialization ==
* group Receiver task context
*   TimerUser->SwTimerLiaison: Register SwTimer
*   SwTimerLiaison->FreeRTOS_Timer: Create FreeRTOS timer
*   SwTimerLiaison->SwTimer: Setup SwTimer
* end
* == Usage ==
* group Receiver task context
*   TimerUser->SwTimer: Start Timer
*   SwTimer->FreeRTOS_Timer: Start FreeRTOS timer
* end
* ...Time passes until timer expires...
* group FreeRTOS timer daemon task context
*   FreeRTOS_Timer->SwTimerLiaison: Expired callback (to TimerExpiredLiaisonCallback)
*   SwTimerLiaison->ReceiverTask: Task Notification
* end
* group Receiver task context
* ReceiverTask->SwTimerLiaison: SwTimerLiaisonNotificationHandler
* SwTimerLiaison->SwTimer: Look up callback
* SwTimerLiaison->TimerUser: Perform callback
* end
* @enduml
*/

/* Z-Wave includes */
#include <SwTimerPrivate.h>

/* FreeeRTOS includes*/
#include <SwTimerLiaison.h>

//#define DEBUGPRINT
#include "DebugPrint.h"
#include "Assert.h"

typedef struct {
  StaticEventGroup_t ReceiverEvent;        /**< Static buffer for FreeRTOS event group */
  EventGroupHandle_t ReceiverEventHandle;  /**< FreeRTOS Event handle to keep track of 
                                                pending timer callbacks */
} SSwTimerLiaisonPrivate_t;

STATIC_ASSERT(sizeof(((struct SSwTimerLiaison *)0)->dummy) == sizeof(SSwTimerLiaisonPrivate_t), STATIC_ASSERT_FAILED_SwTimerLiaison_types_does_not_match_in_size);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static const uint8_t iEventGroupMaxEvents = 24; // FreeRTOS limitation
                                        // (Can be 8 dependent on FreeRTOS config,
                                        // 8 is sometimes used on non 32 bit processors.

void TimerLiaisonInit(
                        SSwTimerLiaison* pThis,
                        uint32_t iTimerPointerArraySize,
                        SSwTimer** pTimerPointerArray,
                        uint8_t iTaskNotificationBitNumber,
                        void* ReceiverTask
                      )
{
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;
  pThis->iTimerCount = 0;
  pThis->iTimerListSize = iTimerPointerArraySize;
  pThis->pTimerList = pTimerPointerArray;  
  pThis->iTaskNotificationBitNumber = iTaskNotificationBitNumber;
  pThis->ReceiverTask = ReceiverTask;

  pTimerLiaisonPrivate->ReceiverEventHandle = xEventGroupCreateStatic(&pTimerLiaisonPrivate->ReceiverEvent);
}

void TimerLiaisonNotificationHandler(SSwTimerLiaison* pThis)
{
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;
  EventBits_t TimerEvents = xEventGroupClearBits(pTimerLiaisonPrivate->ReceiverEventHandle, 0x00FFFFFF);
  
//  DPRINTF("Timer Notification Received on timers %d\r\n", TimerEvents);

  // Check which Timers events are pending 
  for (uint32_t i = 0; i < pThis->iTimerCount; i++)
  {
    if ((TimerEvents & (1 << i)))  // If bit i set
    {
      // Timer i has pending event
      SSwTimer* pTimer = pThis->pTimerList[i];
      ASSERT(pThis == pTimer->pLiaison);  // Check Timer and Liaison is matched

      if (pTimer->pCallback != NULL)
      {
        pTimer->pCallback(pTimer);  // Perform callback
      }      
    }
  }
}

void TimerLiaisonClearPendingTimerEvent(SSwTimerLiaison* pThis, uint32_t TimerId)
{
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;
  ASSERT(TimerId < iEventGroupMaxEvents); // Only so many timers allowed - one timer per event group bit
  xEventGroupClearBits(pTimerLiaisonPrivate->ReceiverEventHandle, (1 << TimerId));
}

void TimerLiaisonClearPendingTimerEventFromISR(SSwTimerLiaison* pThis, uint32_t TimerId)
{
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;
  ASSERT(TimerId < iEventGroupMaxEvents); // Only so many timers allowed - one timer per event group bit
  xEventGroupClearBitsFromISR(pTimerLiaisonPrivate->ReceiverEventHandle, (1 << TimerId));
}

bool TimerLiaisonHasPendingTimerEvent(SSwTimerLiaison* pThis, uint32_t TimerId)
{
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;
  ASSERT(TimerId < iEventGroupMaxEvents); // // Only so many timers allowed - one timer per event group bit

  uint32_t EventBits = xEventGroupGetBits(pTimerLiaisonPrivate->ReceiverEventHandle);

  return (EventBits & (1 << TimerId));
}

bool TimerLiaisonHasPendingTimerEventFromISR(SSwTimerLiaison* pThis, uint32_t TimerId)
{
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;
  ASSERT(TimerId < iEventGroupMaxEvents); // // Only so many timers allowed - one timer per event group bit

  uint32_t EventBits = xEventGroupGetBitsFromISR(pTimerLiaisonPrivate->ReceiverEventHandle);

  return (EventBits & (1 << TimerId));
}

ESwTimerLiaisonStatus TimerLiaisonRegister(SSwTimerLiaison* pThis,
  SSwTimer* pTimer,
  bool bAutoReload,
  void(*pCallback)(SSwTimer* pTimer)
  )
{
  SSwTimerPrivate_t *pTimerPrivate = (SSwTimerPrivate_t*)&pTimer->dummy;
  if (pTimer->pLiaison) // if Timer already registered
  {
    // Assert if Already registered to another TimerLiaison
    // As un-registrering is not supported, registrering to multiple
    // Liaisons is a no no
    ASSERT(pThis == pTimer->pLiaison);
    DPRINTF("Timer %d re-registration\r\n", pTimer->Id);
    return ESWTIMERLIAISON_STATUS_ALREADY_REGISTRERED;
  }

  if (
      (pThis->iTimerCount >= pThis->iTimerListSize) ||
      (pThis->iTimerCount >= iEventGroupMaxEvents)
     )
  {
    return ESWTIMERLIAISON_STATUS_LIST_FULL;
  }

  // Period will be changed before timer start, but set to 1, as 0 is illegal.
  // Throw away returned handle, its really pTimer.
  pTimer->TimerHandle = xTimerCreateStatic("",
    1,
    bAutoReload ? pdTRUE : pdFALSE,
    (void *)0, (TimerCallbackFunction_t)TimerLiaisonExpiredTimerCallback,
    &pTimerPrivate->xTimer
    );
  pTimer->pLiaison = pThis;
  pTimer->pCallback = pCallback;
  pTimer->Id = pThis->iTimerCount;
  pThis->pTimerList[pThis->iTimerCount] = pTimer;
  pThis->iTimerCount++;

  DPRINTF("Timer %d registrered  listsize: %d\r\n", pThis->iTimerCount, pThis->iTimerListSize);

  return ESWTIMERLIAISON_STATUS_SUCCESS;
}

void TimerLiaisonSetReceiverTask(SSwTimerLiaison* pThis, void* ReceiverTask)
{
  pThis->ReceiverTask = ReceiverTask;
}


// Static assert ensures that pTimer == FreeRTOS TimerHandle
STATIC_ASSERT(offsetof(SSwTimerPrivate_t, xTimer) == 0, STATIC_ASSERT_FAILED_StaticTimer_t_must_be_first_member_of_STIMER);

void TimerLiaisonExpiredTimerCallback(SSwTimer* pTimer)
{
  // Get TimerLiaison object
  SSwTimerLiaison* pThis = pTimer->pLiaison;
  SSwTimerLiaisonPrivate_t *pTimerLiaisonPrivate = (SSwTimerLiaisonPrivate_t *)&pThis->dummy;

  DPRINTF("Timer %d Timeout, pCallback=%p\r\n", pTimer->Id, pTimer->pCallback);
  uint32_t EventBit = 1 << (pTimer->Id);
  xEventGroupSetBits(pTimerLiaisonPrivate->ReceiverEventHandle, EventBit);
  uint32_t TimerTaskNotification = 1 << pThis->iTaskNotificationBitNumber;
  ASSERT(pThis->ReceiverTask);
  uint32_t Status = xTaskNotify(pThis->ReceiverTask, TimerTaskNotification, eSetBits);

  if (Status != pdPASS)
  {
    DPRINTF("Timer notification failed - Timer ID %d, Liaison %08X\r\n", pTimer->Id, pThis);
  }
}
