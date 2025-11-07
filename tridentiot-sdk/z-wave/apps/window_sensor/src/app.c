/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * TridentIoT Window/Door sensor demo app
 */

#include <stdbool.h>
#include <stdint.h>
#include "Assert.h"
#include "MfgTokens.h"
#include "DebugPrintConfig.h"
//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#include "AppTimer.h"
#include "ZW_system_startup_api.h"
#include "CC_Basic.h"
#include "CC_Battery.h"
#include "CC_Notification.h"
#include "cc_notification_config_api.h"
#include "ZAF_Common_helper.h"
#include "ZAF_Common_interface.h"
#include "ZAF_network_learn.h"
#include "ZAF_network_management.h"
#include "events.h"
#include "zpal_watchdog.h"
#include "app_hw.h"
#include "board_indicator.h"
#include "ZAF_ApplicationEvents.h"
#include "zaf_event_distributor_soc.h"
#include "cc_agi_config_api.h"
#include "zpal_misc.h"
#include "zw_build_no.h"
#include "zaf_protocol_config.h"
#include "ZW_TransportEndpoint.h"
#ifdef DEBUGPRINT
#include "ZAF_PrintAppInfo.h"
#endif

#ifdef DEBUGPRINT
static uint8_t m_aDebugPrintBuffer[96];
// Debug port is defined in apps hardware file
extern zpal_debug_config_t debug_port_cfg;
#endif


void ApplicationTask(SApplicationHandles* pAppHandles);


/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
ZW_APPLICATION_STATUS
ApplicationInit(__attribute__((unused)) zpal_reset_reason_t eResetReason)
{
  SRadioConfig_t* RadioConfig;

  DPRINT("Enabling watchdog\n");
  zpal_enable_watchdog(true);

#ifdef DEBUGPRINT
  zpal_debug_init(debug_port_cfg);
  DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), zpal_debug_output);
  DebugPrintf("ApplicationInit eResetReason = %d\n", eResetReason);
#endif // DEBUGPRINT

  RadioConfig = zaf_get_radio_config();

  // Read Rf region from MFG_ZWAVE_COUNTRY_FREQ
  zpal_radio_region_t regionMfg;
  ZW_GetMfgTokenDataCountryFreq((void*) &regionMfg);
  if (isRfRegionValid(regionMfg)) {
    RadioConfig->eRegion = regionMfg;
  } else {
    ZW_SetMfgTokenDataCountryRegion((void*) &RadioConfig->eRegion);
  }

  /*************************************************************************************
   * CREATE USER TASKS  -  ZW_ApplicationRegisterTask() and ZW_UserTask_CreateTask()
   *************************************************************************************
   * Register the main APP task function.
   *
   * ATTENTION: This function is the only task that can call ZAF API functions!!!
   * Failure to follow guidelines will result in undefined behavior.
   *
   * Furthermore, this function is the only way to register Event Notification
   * Bit Numbers for associating to given event handlers.
   *
   * ZW_UserTask_CreateTask() can be used to create additional tasks.
   * @see zwave_soc_sensor_pir example for more info.
   *************************************************************************************/
  bool bWasTaskCreated = ZW_ApplicationRegisterTask(
                                                    ApplicationTask,
                                                    EAPPLICATIONEVENT_ZWRX,
                                                    EAPPLICATIONEVENT_ZWCOMMANDSTATUS,
                                                    zaf_get_protocol_config()
                                                    );
  ASSERT(bWasTaskCreated);

  return(APPLICATION_RUNNING);
}

/**
 * A pointer to this function is passed to ZW_ApplicationRegisterTask() making it the FreeRTOS
 * application task.
 */
void
ApplicationTask(SApplicationHandles* pAppHandles)
{
  zpal_reset_reason_t resetReason;

  DPRINT("\r\nWindow sensor Main App/Task started! \n");

  ZAF_Init(xTaskGetCurrentTaskHandle(), pAppHandles);

#ifdef DEBUGPRINT
  ZAF_PrintAppInfo();
#endif

  app_hw_init();

  resetReason = GetResetReason();

  /* Check the battery level.
   * If required, go to TRANSMIT state to send the report to the lifeline.
   * The Battery Report must be sent out before the WakeUp Notification. Therefore this function
   * must called prior to anything CC Wakeup related.
   */
  if (true == cc_battery_check_level_changed())
  {
    zaf_event_distributor_enqueue_app_event(EVENT_APP_BATTERY_REPORT);
  }

  /* Re-load and process Deep Sleep persistent application timers.
   * NB: Before calling AppTimerDeepSleepPersistentLoadAll here all
   *     application timers must have been been registered with
   *     AppTimerRegister() or AppTimerDeepSleepPersistentRegister().
   *     Essentially it means that all CC handlers must be
   *     initialized first.
   */
  AppTimerDeepSleepPersistentLoadAll(resetReason);

  if (ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT == resetReason)
  {
    app_hw_deep_sleep_wakeup_handler();
  }

  /**
   * Set the maximum inclusion request interval for SmartStart.
   * Valid range 0 and 5-99. 0 is default value and corresponds to 512 sec.
   * The range 5-99 corresponds to 640-12672sec in units of 128sec/tick in between.
   */
  ZAF_SetMaxInclusionRequestIntervals(0);

  if(ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT != resetReason)
  {
    /* Enter SmartStart*/
    /* Protocol will commence SmartStart only if the node is NOT already included in the network */
    ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
  }

  DPRINTF("\r\nIsWakeupCausedByRtccTimeout=%s", (IsWakeupCausedByRtccTimeout()) ? "true" : "false");
  DPRINTF("\r\nCompletedSleepDurationMs   =%u", GetCompletedSleepDurationMs());

  // Wait for and process events
  DPRINT("\r\nWindow/Door sensor Event processor Started\n");
  for (;;)
  {
    zaf_event_distributor_distribute();
  }
}

/**
 * @brief The core state machine of this sample application.
 * @param event The event that triggered the call of zaf_event_distributor_app_event_manager.
 */
void
zaf_event_distributor_app_event_manager(const uint8_t event)
{
  DPRINTF("zaf_event_distributor_app_event_manager Ev: 0x%02x\r\n", event);

  switch (event) {
    case EVENT_APP_BATTERY_REPORT:
      /* BATTERY REPORT EVENT received. Send a battery level report */
      DPRINT("\r\nBattery Level report transmit\n");
      (void) CC_Battery_LevelReport_tx(NULL, ENDPOINT_ROOT, NULL);
      break;
    case EVENT_APP_WINDOW_OPEN:
      (void) CC_Notification_TriggerAndTransmit(0,
                                                NOTIFICATION_EVENT_ACCESS_CONTROL_DOOR_IS_OPEN,
                                                NULL,
                                                0,
                                                NULL,
                                                false);
      DPRINT("\r\n");
      DPRINT("\r\n      *!*!**!*!**!*!**!*!**!*!**!*!**!*!**!*!*");
      DPRINT("\r\n      *!*!*       WINDOW OPEN            *!*!*");
      DPRINT("\r\n      *!*!**!*!**!*!**!*!**!*!**!*!**!*!**!*!*");
      DPRINT("\r\n");

      break;
    case EVENT_APP_WINDOW_CLOSE:
      (void) CC_Notification_TriggerAndTransmit(0,
                                                NOTIFICATION_EVENT_ACCESS_CONTROL_DOOR_IS_CLOSED,
                                                NULL,
                                                0,
                                                NULL,
                                                false);
      DPRINT("\r\n");
      DPRINT("\r\n      *!*!**!*!**!*!**!*!**!*!**!*!**!*!**!*!*");
      DPRINT("\r\n      *!*!*         WINDOW CLOSE         *!*!*");
      DPRINT("\r\n      *!*!**!*!**!*!**!*!**!*!**!*!**!*!**!*!*");
      DPRINT("\r\n");
      break;
    default:
      break;
  }
}

void
zaf_nvm_app_reset(void)
{
  AppTimerDeepSleepPersistentResetStorage();
}
