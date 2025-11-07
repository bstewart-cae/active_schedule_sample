/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file zniffer_startup.c
 * @brief Z-Wave zniffer application startup module
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#include <Assert.h>

#include <FreeRTOS.h>
#include "task.h"

// #define DEBUGPRINT // NOSONAR
// #include "DebugPrint.h"
// #include "DebugPrintConfig.h"

#include <zniffer_app.h>
#include <comm_interface.h>

#include <zpal_radio.h>
#include <zpal_watchdog.h>
#include <zpal_power_manager.h>
#include <zpal_radio_utils.h>
#include <zpal_entropy.h>
#include <zpal_init.h>
#include <zpal_misc.h>
#include <zpal_uart.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static bool m_schedulerIsStarted = false;

/*
 * Main task stack
 */
// Task and stack buffer allocation for the Z-Wave task!
static StackType_t StackBuffer[APPLICATION_TASK_STACK];
static StaticTask_t TaskBuffer;

static bool wdog_enabled = false;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                              STUB FUNCTIONS                              */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*============================== system_startup ===============================
**  Startup the system, This function is the first call from the startup code
**----------------------------------------------------------------------------*/
int app_system_startup()
{
  m_schedulerIsStarted = false;  // The FreeRTOS Scheduler is set to be in NOT STARTED state!

  /* Create the application task */
  TaskHandle_t xHandle = xTaskCreateStatic(
                                  (TaskFunction_t)&ZwaveZnifferTask,  // pvTaskCode
                                  "Zniffer",                          // pcName
                                  APPLICATION_TASK_STACK,             // usStackDepth
                                  NULL,                               // pvParameters
                                  APPLICATION_TASK_PRIORITY_STACK,    // uxPriority
                                  StackBuffer,                        // pxStackBuffer
                                  &TaskBuffer                         // pxTaskBuffer
                                );
  ASSERT(xHandle != NULL);

  /* Start the scheduler. */
  m_schedulerIsStarted = true;  // Indicate that the Scheduler has started!

  return 0;
}

/*============================== system_startup ===============================
**  Startup function called from main()
**----------------------------------------------------------------------------*/
void zpal_system_startup(__attribute__((unused)) zpal_reset_reason_t reset_reason)
{
  app_system_startup();
}

void
RegionChangeHandler(__attribute__((unused)) zpal_radio_event_t regionChangeStatus)
{
}

bool ZW_system_startup_IsSchedulerStarted(void)
{
  return m_schedulerIsStarted;
}

void zpal_zw_pm_event_handler(zpal_pm_mode_t from, zpal_pm_mode_t to)
{
}

void enterPowerDown(__attribute__((unused)) uint32_t millis)
{
  wdog_enabled = zpal_is_watchdog_enabled();

  if (wdog_enabled)
  {
    zpal_enable_watchdog(false);
  }
}

void exitPowerDown(__attribute__((unused)) uint32_t millis)
{
  if (wdog_enabled)
  {
    zpal_enable_watchdog(wdog_enabled);
  }
}
