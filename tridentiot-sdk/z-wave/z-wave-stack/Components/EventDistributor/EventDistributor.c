// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* Event Distributor module
*
* @copyright 2018 Silicon Laboratories Inc.
*/


/* Z-Wave includes */
#include <EventDistributor.h>

/* FreeeRTOS includes*/
#include <FreeRTOS.h>
#include <task.h>

#include "Assert.h"


EEventDistributorStatus EventDistributorConfig(
                                            SEventDistributor* pThis,
                                            uint8_t iEventHandlerTableSize,
                                            const EventDistributorEventHandler* pEventHandlerTable,
                                            void(*pNoEvent)(void)
                                          )
{
  if (iEventHandlerTableSize > 32)
  {
    return EEVENTDISTRIBUTOR_STATUS_TABLE_TOO_LARGE;
  }

  if (pThis->iEventHandlerTableSize > iEventHandlerTableSize)
  {
    // If existing table is LARGER than new table, write table size BEFORE table
    pThis->iEventHandlerTableSize = iEventHandlerTableSize;
    pThis->pEventHandlerTable = pEventHandlerTable;    
  }
  else
  {
    // If existing table is SMALLER than new table, write table size AFTER table
    pThis->pEventHandlerTable = pEventHandlerTable;
    pThis->iEventHandlerTableSize = iEventHandlerTableSize;
  }

  pThis->pNoEvent = pNoEvent;
  
  return EEVENTDISTRIBUTOR_STATUS_SUCCESS;
}


uint32_t EventDistributorDistribute(
  const SEventDistributor* pThis,
  uint32_t iEventWait,
  uint32_t NotificationClearMask
  )
{
  // Create bit mask marking task notification bits handled by EventDistributor
  uint32_t EventsBitMask = 0;
  if (0 != pThis->iEventHandlerTableSize)
  {
    EventsBitMask = 0xFFFFFFFF >> (32 - pThis->iEventHandlerTableSize);
  }
  
  uint32_t NotificationsPending = 0;
  // Clear handled(EventsBitMask) and requested(NotificationClearMask)
  // notification bits on wait exit (EventsBitMask)
  xTaskNotifyWait(
                  0,
                  EventsBitMask | NotificationClearMask,
                  &NotificationsPending,
                  pdMS_TO_TICKS(iEventWait)
                );

  uint32_t EventsPending = NotificationsPending & EventsBitMask;  // Remove unhandled bits
  if (0 == EventsPending)
  {
    if (pThis->pNoEvent)
    {
      pThis->pNoEvent();
    }
  }
  else
  {
    /* Will execute all set events generated at this time
     * in the order presented in the EventHandlerTable structure! */
    for (uint32_t i = 0; i < pThis->iEventHandlerTableSize; i++)
    {
      if (((EventsPending >> i) & 0x01) != 0)  // If Event is set
      {
        pThis->pEventHandlerTable[i]();        // Call event handler
      }
    }
  }

  // Return notification bits not handled by EventDistributor
  return NotificationsPending & ~EventsBitMask;
}


