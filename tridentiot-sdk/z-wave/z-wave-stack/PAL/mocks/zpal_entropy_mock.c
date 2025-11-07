// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include <mock_control.h>
#include <zpal_entropy.h>

#define MOCK_FILE "zpal_entropy_mock.c"


zpal_status_t zpal_get_random_data(uint8_t *data, size_t len)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_FAIL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, data, len);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, data);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, len);

  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[0].p, data, len, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

uint8_t zpal_get_pseudo_random(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}
