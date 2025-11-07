/// ****************************************************************************
/// @file zwave_api_interface.c
///
/// @brief queue based interface to the z-wave API
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************


#include <inttypes.h>
#include "SizeOf.h"
#include "events.h"
#include <zaf_event_distributor_soc.h>
#include <ZW_application_transport_interface.h>
#include <ZAF_Common_interface.h>
#include <ZAF_Common_helper.h>
#include <ZAF_ApplicationEvents.h>

static
/*
 this function waits for a resposne from invoked protocol API.
*/
uint8_t get_command_response(SZwaveCommandStatusPackage *pCmdStatus, EZwaveCommandStatusType cmdType)
{
  const SApplicationHandles *m_pAppHandles = ZAF_getAppHandle();
  TaskHandle_t m_pAppTaskHandle = ZAF_getAppTaskHandle();
  QueueHandle_t Queue = m_pAppHandles->ZwCommandStatusQueue;

  for (uint8_t delayCount = 0; delayCount < 100; delayCount++)
  {
    UBaseType_t QueueElmCount = uxQueueMessagesWaiting(Queue);
    while (QueueElmCount--)
    {
      if (!xQueueReceive(Queue, (uint8_t *)pCmdStatus, 0))
      {
        continue;
      }

      if (pCmdStatus->eStatusType != cmdType)
      {
        BaseType_t result = xQueueSendToBack(Queue, (uint8_t *)pCmdStatus, 0);
        ASSERT(pdTRUE == result);
        continue;
      }

      // Matching status type found
      if (m_pAppTaskHandle && uxQueueMessagesWaiting(Queue) > 0)
      {
        BaseType_t Status = xTaskNotify(m_pAppTaskHandle, 1 << EAPPLICATIONEVENT_ZWCOMMANDSTATUS, eSetBits);
        ASSERT(Status == pdPASS);
      }

      return true;
    }

    vTaskDelay(10);
  }

  if (m_pAppTaskHandle && uxQueueMessagesWaiting(Queue) > 0)
  {
    BaseType_t Status = xTaskNotify(m_pAppTaskHandle, 1 << EAPPLICATIONEVENT_ZWCOMMANDSTATUS, eSetBits);
    ASSERT(Status == pdPASS);
  }

  return false;
}

/*
  This function register a callback function into the protocol.
  The callback function is called before entering the deep sleep power mode.
  The fucntion retruns true if the callback registed, else returns false.
*/
bool set_powerdown_callback(zaf_wake_up_callback_t callback)
{
  SZwaveCommandPackage cmdPackage = {.eCommandType = EZWAVECOMMANDTYPE_PM_SET_POWERDOWN_CALLBACK,
                                     .uCommandParams.PMSetPowerDownCallback.callback = callback
                                    };

  EQueueNotifyingStatus QueueStatus = QueueNotifyingSendToBack(ZAF_getZwCommandQueue(), (uint8_t *)&cmdPackage, 0);
  ASSERT(EQUEUENOTIFYING_STATUS_SUCCESS == QueueStatus);
  SZwaveCommandStatusPackage cmdStatus = {.eStatusType = 0};
  if (get_command_response(&cmdStatus, EZWAVECOMMANDSTATUS_PM_SET_POWERDOWN_CALLBACK))
  {
    return cmdStatus.Content.SetPowerDownCallbackStatus.result;
  }
  ASSERT(false);
  return false;
}