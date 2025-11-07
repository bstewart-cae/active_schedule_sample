/**
 * Z-Wave Certified Application LED Bulb
 *
 * @copyright 2020 Silicon Laboratories Inc.
 */

#include <stdbool.h>
#include <stdint.h>
#include "MfgTokens.h"
#include "DebugPrintConfig.h"
//#define DEBUGPRINT
#include "DebugPrint.h"
#include "ZW_system_startup_api.h"
#include "CC_MultilevelSwitch_Support.h"
#include "ZAF_Common_helper.h"
#include "ZAF_Common_interface.h"
#include "ZAF_network_learn.h"
#include "events.h"
#include "zpal_watchdog.h"
#include "app_hw.h"
#include "board_indicator.h"
#include "zw_region_config.h"
#include "ZAF_ApplicationEvents.h"
#include "zaf_event_distributor_soc.h"
#include "zpal_misc.h"
#include "zw_build_no.h"
#include "zaf_protocol_config.h"
#ifdef DEBUGPRINT
#include "ZAF_PrintAppInfo.h"
#endif

#ifdef DEBUGPRINT
static uint8_t m_aDebugPrintBuffer[96];
// Debug port is defined in apps hardware file
extern zpal_debug_config_t debug_port_cfg;
#endif

static void ApplicationTask(SApplicationHandles* pAppHandles);

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
ZW_APPLICATION_STATUS ApplicationInit(__attribute__((unused)) zpal_reset_reason_t eResetReason)
{
  SRadioConfig_t* RadioConfig;

  zpal_watchdog_init();
  zpal_enable_watchdog(true);

#ifdef DEBUGPRINT
  zpal_debug_init(debug_port_cfg);
  DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), zpal_debug_output);
  DebugPrintf("ApplicationInit eResetReason = %d\n", eResetReason);
#endif

  RadioConfig = zaf_get_radio_config();

  // Read Rf region from MFG_ZWAVE_COUNTRY_FREQ
  zpal_radio_region_t regionMfg;
  ZW_GetMfgTokenDataCountryFreq((void*) &regionMfg);
  if (isRfRegionValid(regionMfg)) {
    RadioConfig->eRegion = regionMfg;
  } else {
    ASSERT(isRfRegionValid(RadioConfig->eRegion));
  }

  /*
   * Register the main application task.
   *
   * Attention: this is the only FreeRTOS task that can invoke the ZAF API.
   *
   * ZW_UserTask_CreateTask() can be used to create additional application tasks. See the
   * Sensor PIR application for an example use of ZW_UserTask_CreateTask().
   */
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
static void ApplicationTask(SApplicationHandles* pAppHandles)
{
  uint32_t unhandledEvents = 0;
  ZAF_Init(xTaskGetCurrentTaskHandle(), pAppHandles);

#ifdef DEBUGPRINT
  ZAF_PrintAppInfo();
#endif

  app_hw_init();

  /* Enter SmartStart*/
  /* Protocol will commence SmartStart only if the node is NOT already included in the network */
  ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  // Wait for and process events
  DPRINT("LED Bulb Event processor Started\r\n");
  for (;; ) {
    unhandledEvents = zaf_event_distributor_distribute();
    if (0 != unhandledEvents) {
      DPRINTF("Unhandled Events: 0x%08lx\n", unhandledEvents);
#ifdef UNIT_TEST
      return;
#endif
    }
  }
}

/**
 * @brief The core state machine of this sample application.
 * @param event The event that triggered the call of zaf_event_distributor_app_event_manager.
 */
void
zaf_event_distributor_app_event_manager(const uint8_t event)
{
  DPRINTF("zaf_event_distributor_app_event_manager Ev: %d\r\n", event);

  switch(event)
  {
    default:
      // Unknow event - do nothing.
      break;
  }
}
