// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* Notifying Queue module.
*
* @copyright 2018 Silicon Laboratories Inc.
*/

/* Z-Wave includes */
#include <QueueNotifying.h>

/* FreeeRTOS includes*/
#include "FreeRTOS.h"
#include <task.h>
#include <queue.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

#include "Assert.h"

static EQueueNotifyingStatus QueueNotifyingSend(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem,
                                                uint32_t iTimeToWait,
                                                bool bSendToBack
                                               );

static EQueueNotifyingStatus QueueNotifyingSendFromISR(
    SQueueNotifying* pThis,
    const uint8_t* pItem,
    bool bSendToBack
    );

void QueueNotifyingInit(SQueueNotifying* pThis,
                        void *Queue,
                        void * ReceiverTask,
                        uint8_t iTaskNotificationBitNumber)
{
  ASSERT(Queue);
  ASSERT(ReceiverTask);

  pThis->Queue = Queue;
  pThis->ReceiverTask = ReceiverTask;
  pThis->iTaskNotificationBitNumber = iTaskNotificationBitNumber;
}

EQueueNotifyingStatus QueueNotifyingSendToBack(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem,
                                                uint32_t iTimeToWait
                                              )
{
  return QueueNotifyingSend(pThis, pItem, iTimeToWait, true);
}

EQueueNotifyingStatus QueueNotifyingSendToFront(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem,
                                                uint32_t iTimeToWait
                                                )
{
  return QueueNotifyingSend(pThis, pItem, iTimeToWait, false);
}

static EQueueNotifyingStatus QueueNotifyingSend(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem,
                                                uint32_t iTimeToWait,
                                                bool bSendToBack
                                               )
{
  BaseType_t Status;

  if (bSendToBack)
  {
    Status = xQueueSendToBack((QueueHandle_t)pThis->Queue, pItem, pdMS_TO_TICKS(iTimeToWait));
  }
  else
  {
    Status = xQueueSendToFront((QueueHandle_t)pThis->Queue, pItem, pdMS_TO_TICKS(iTimeToWait));
  }

  /* Removed the assert here as there is no need & we are returning timeout errorcode in case of queue is full */

  if (Status == pdPASS)
  {
    if (pThis->ReceiverTask)
    {
      Status = xTaskNotify((TaskHandle_t)pThis->ReceiverTask, (1 << pThis->iTaskNotificationBitNumber), eSetBits);
      ASSERT(Status == pdPASS); // We probably received a bad Task handle
    }

    return EQUEUENOTIFYING_STATUS_SUCCESS;
  }

  return EQUEUENOTIFYING_STATUS_TIMEOUT;
}


EQueueNotifyingStatus QueueNotifyingSendToBackFromISR(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem
                                              )
{
  return QueueNotifyingSendFromISR(pThis, pItem, true);
}

EQueueNotifyingStatus QueueNotifyingSendToFrontFromISR(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem
                                                )
{
  return QueueNotifyingSendFromISR(pThis, pItem, false);
}

static EQueueNotifyingStatus QueueNotifyingSendFromISR(
                                                SQueueNotifying* pThis,
                                                const uint8_t* pItem,
                                                bool bSendToBack
                                               )
{
  BaseType_t Status;
  BaseType_t xHigherPriorityTaskWoken;

  xHigherPriorityTaskWoken = pdFALSE;
  if (bSendToBack)
  {
    Status = xQueueSendToBackFromISR((QueueHandle_t)pThis->Queue, pItem, &xHigherPriorityTaskWoken);
  }
  else
  {
    Status = xQueueSendToFrontFromISR((QueueHandle_t)pThis->Queue, pItem, &xHigherPriorityTaskWoken);
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  if (Status == pdPASS)
  {
    if (pThis->ReceiverTask)
    {
      xHigherPriorityTaskWoken = pdFALSE;
      Status = xTaskNotifyFromISR((TaskHandle_t)pThis->ReceiverTask, (1 << pThis->iTaskNotificationBitNumber), eSetBits, &xHigherPriorityTaskWoken);
      ASSERT(Status == pdPASS); // We probably received a bad Task handle
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return EQUEUENOTIFYING_STATUS_SUCCESS;
  }

  return EQUEUENOTIFYING_STATUS_TIMEOUT;
}

