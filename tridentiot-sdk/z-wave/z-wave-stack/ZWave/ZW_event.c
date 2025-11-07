// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave event handling module
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_event.h>
#include <string.h>
//#define DEBUGPRINT
#include <DebugPrint.h>
#include <Assert.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


typedef struct _eventQueueElement_
{
  uint8_t event;
  uint8_t status;
} eventQueueElement;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/


eventQueueElement eventQueue[EVENT_QUEUE_SIZE];
uint8_t eventQueueCount;


/****************************************************************************/
/*                            Functions used in library                     */
/****************************************************************************/


/*================================== EventPush ===============================
**  Function to push event into event queue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
EventPush(
  uint8_t bEvent,
  uint8_t bStatus)
{
  DPRINTF("EventPush - Event %X, Status %X", bEvent, bStatus);

#if 0
  /* Make sure that TICKS only use one event slot at a time */
  if (bEvent == EVENT_TIMEOUT)
  {
    for (uint32_t i = 0; i < EVENT_QUEUE_SIZE; i++)
    {
      if (eventQueue[i].event == EVENT_TIMEOUT)
      {
        eventQueue[i].status += bStatus;
        return true;
      }
    }
  }
#endif
  if (eventQueueCount < EVENT_QUEUE_SIZE)
  {
    DPRINT(" - Pushed\r\n");
    /* There is room for the event */
    eventQueue[eventQueueCount].event = bEvent;
    eventQueue[eventQueueCount++].status = bStatus;
    return true;
  }

  DPRINT("\r\n");
  return false;
}


/*================================== EventPop ================================
**  Function to pop the first event in the event queue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
EventPop(
  uint8_t *pbEvent,
  uint8_t *pbStatus)
{
  if (eventQueueCount)
  {
    *pbEvent = eventQueue[0].event;
    *pbStatus = eventQueue[0].status;

    // Static assert to protect agains incorrect use of memcpy.
    // memcpy does not support overlapping source and destination
    STATIC_ASSERT(EVENT_QUEUE_SIZE == 2, STATIC_ASSERT_FAILED_event_queue_only_supports_size_2);
    memcpy((uint8_t*)&eventQueue[0], (uint8_t*)&eventQueue[1], sizeof(eventQueueElement) * (EVENT_QUEUE_SIZE - 1));
#if 0
    eventQueue[EVENT_QUEUE_SIZE - 1].event = EVENT_IDLE;
#endif
    eventQueueCount--;
    DPRINTF("EventPop  - Event %X, Status %X, Count %X\r\n", *pbEvent, *pbStatus, eventQueueCount);
    return true;
  }
  *pbEvent = EVENT_IDLE;
  return false;
}


/*================================== EventPeek ===============================
**  Function to peek onto the first entry in the event queue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
EventPeek(
  uint8_t *pbEvent,
  uint8_t *pbStatus)
{
  if (eventQueueCount)
  {
    *pbEvent = eventQueue[0].event;
    *pbStatus = eventQueue[0].status;
    return true;
  }
  *pbEvent = EVENT_IDLE;
  return false;
}

