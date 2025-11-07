// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave event handling module
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */
#ifndef _ZW_EVENT_H_
#define _ZW_EVENT_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

#define EVENT_QUEUE_SIZE  2

#define EVENT_IDLE        0
#define EVENT_TIMEOUT     1


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern uint8_t eventQueueCount;

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * Pushes an event and a corresponding status to the event queue.
 *
 * @param[in] bEvent The event to push.
 * @param[in] bStatus A corresponding status that can take any 8 bit value.
 * @return Returns true if the event was pushed succesfully, false otherwise.
 */
bool
EventPush(
  uint8_t bEvent,
  uint8_t bStatus);


/*================================== EventPop ================================
**  Function to pop the first event in the event queue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
EventPop(
  uint8_t *pbEvent,
  uint8_t *pbStatus);


/*================================== EventPeek ===============================
**  Function to peek onto the first entry in the event queue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
EventPeek(
  uint8_t *pbEvent,
  uint8_t *pbStatus);


#endif /* _ZW_EVENT_H_ */
