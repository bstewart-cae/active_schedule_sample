// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_timer_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
//#include "example_test_mock.h"
#include <ZW_typedefs.h>
//#include <stdint.h>
#include "mock_control.h"
#include "unity.h"

#define MOCK_FILE "ZW_transport_timer_mock.c"

void TransportTimerStart(VOID_CALLBACKFUNC(func)(), uint16_t wtimerTicks)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, func, wtimerTicks);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, func);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG1, wtimerTicks);
}

void TransportTimerCancel(void)
{
  mock_t * p_mock;
  
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void TransportTimerRestart(void)
{
  mock_t * p_mock;
  
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void TransportTimerPoll(void)
{
  mock_t * p_mock;
  
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void TransportTimerInit(void)
{
  mock_t * p_mock;
  
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}
