// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file Assert_zw.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stddef.h>
#include <Assert.h>
#include <DebugPrint.h>
#include <zpal_watchdog.h>
#include <zpal_misc.h>
#include "ZW_classcmd.h"

static AssertCb_t AssertCb = NULL;

void Assert(const char* pFileName, int iLineNumber)
{
  zpal_disable_interrupts();

  DebugPrintf("Assertion failed: File %s, line %d\n", pFileName, iLineNumber);

  /* Disable the watch dog. Asserts are only included in debug builds, so we
    * don't want the system to auto reset before we've had a chance to analyze
    * the call stack. */
  zpal_enable_watchdog(false);

  if(AssertCb) {
    AssertCb();
  }

  for (;;);
}

void Assert_SetCb(AssertCb_t cb)
{
  AssertCb = cb;
}

const void* AssertPtr(const void* p, const char* message)
{
  if (p != NULL) {
    return(p);
  }
  DebugPrintf("assert: nullpointer: %p %s\n", p, message);
#ifndef DEBUG
  DebugPrintf("error: About to reboot: \n");
  zpal_reboot_with_info(MFG_ID_ZWAVE_ALLIANCE, ZPAL_RESET_ASSERT_PTR);
  for (;;);
#endif
  ASSERT(p != NULL);
  return(p);
}

