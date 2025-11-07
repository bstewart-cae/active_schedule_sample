/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file radio_cli_startup.c
 * @brief Z-Wave radio test CLI tool
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#include <Assert.h>

#include <FreeRTOS.h>
#include "task.h"

#include "radio_cli_app.h"

//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#include "DebugPrintConfig.h"

#include <zpal_radio.h>
#include <zpal_watchdog.h>
#include <zpal_power_manager.h>
#include <zpal_radio_utils.h>
#include <zpal_entropy.h>
#include <zpal_init.h>
#include <zpal_misc.h>
#include <zpal_uart.h>

#include "zpal_misc_private.h"

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
// Task and stack buffer allocation for the RadioCli task
static StackType_t StackBuffer[APPLICATION_TASK_STACK] __attribute__((aligned(4)));
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

int app_system_startup(zpal_reset_reason_t reset_reason)
{
  m_schedulerIsStarted = false;  // The FreeRTOS Scheduler is set to be in NOT STARTED state!
  cli_radio_reset_reason_set(reset_reason);

  if ((ZPAL_RESET_REASON_POWER_ON == reset_reason) || (ZPAL_RESET_REASON_OTHER == reset_reason))
  {
    zpal_clear_restarts();
  }
  /* Create the application task */
  TaskHandle_t xHandle = xTaskCreateStatic(
                                  (TaskFunction_t)&ZwaveCliTask,  // pvTaskCode
                                  "RadioCLI",                         // pcName
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
void zpal_system_startup(zpal_reset_reason_t reset_reason)
{
  app_system_startup(reset_reason);
}

void
RegionChangeHandler(__attribute__((unused)) zpal_radio_event_t regionChangeStatus)
{
}

bool ZW_system_startup_IsSchedulerStarted(void)
{
  return m_schedulerIsStarted;
}
