// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_timer_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include "SwTimer.h"

#define MOCK_FILE "ZW_timer_mock.c"

bool ZwTimerRegister(
                SSwTimer* pTimer,
                bool bAutoReload,
                void(*pCallback)(SSwTimer* pTimer)
              )
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(
              p_mock,
		      pTimer,
			  bAutoReload,
			  pCallback);

  MOCK_CALL_COMPARE_INPUT_POINTER(
	          p_mock,
	          ARG0,
			  pTimer);

  MOCK_CALL_COMPARE_INPUT_BOOL(
	          p_mock,
	          ARG1,
			  bAutoReload);

  MOCK_CALL_COMPARE_INPUT_POINTER(
 	          p_mock,
 	          ARG2,
			  pCallback);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);

}
