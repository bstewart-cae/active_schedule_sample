// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_init_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <zpal_init.h>

#define MOCK_FILE "zpal_init_mock.c"


bool zpal_init_is_valid(uint8_t generic_type, uint8_t specific_type)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);

  MOCK_CALL_ACTUAL(p_mock, generic_type, specific_type);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, generic_type);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, specific_type);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void zpal_init_invalidate(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

zpal_reset_reason_t zpal_get_reset_reason(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_RESET_REASON_SOFTWARE);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_RESET_REASON_OTHER);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_reset_reason_t);
}
