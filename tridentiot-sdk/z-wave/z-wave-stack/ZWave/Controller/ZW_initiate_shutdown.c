// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* Implements a method to shutting down of the chip gracefully
* 
* @copyright Copyright 2020 Silicon Laboratories Inc. www.silabs.com, All Rights Reserved
*/
/* Z-Wave includes */
#include "ZW_tx_queue.h"
#include "ZW_timer.h"
#include "ZW_main.h"
#include <zpal_misc.h>

static void (*m_pCallback)(void) = NULL;
static void Initiate_shutdown_cb(void)
{
  zpal_shutdown_handler();
  if (m_pCallback)
  {
    m_pCallback();
  }
}

void 
ZW_initiate_shutdown(void (*callback)(void))
{
  m_pCallback = callback;
  ZwTimerStopAll();
  TxQueueInit();
  zpal_initiate_shutdown_handler();
  ZW_SetAppPowerDownCallback(Initiate_shutdown_cb);
}
