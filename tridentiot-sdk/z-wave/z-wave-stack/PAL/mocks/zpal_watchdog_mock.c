// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_watchdog_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <zpal_watchdog.h>

#define MOCK_FILE "zpal_watchdog_mock.c"

bool zpal_is_watchdog_enabled(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void zpal_enable_watchdog(bool enable)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, enable);

  MOCK_CALL_COMPARE_INPUT_BOOL(p_mock, ARG0, enable);
}

void zpal_feed_watchdog(void)
{
}

void zpal_watchdog_init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}
