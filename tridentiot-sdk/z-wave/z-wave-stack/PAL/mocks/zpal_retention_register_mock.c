// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_retention_register_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <zpal_retention_register.h>
#include <stdbool.h>
#include <mock_control.h>

#define MOCK_FILE "zpal_retention_register_mock.c"

#define RETENTION_REG_COUNT 32

static uint32_t mRetentionRegs[RETENTION_REG_COUNT];


zpal_status_t zpal_retention_register_read_FAKE(uint32_t regIndex, uint32_t *dataPtr)
{
  TEST_ASSERT_NOT_NULL(dataPtr);

  if (regIndex >= RETENTION_REG_COUNT)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }
  if(NULL == dataPtr)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }
  *dataPtr = mRetentionRegs[regIndex];

  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_retention_register_read(uint32_t regIndex, uint32_t *dataPtr)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(zpal_retention_register_read_FAKE, regIndex, dataPtr);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);

  MOCK_CALL_ACTUAL(p_mock, regIndex, dataPtr);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, regIndex);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, dataPtr);

  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[1].p, dataPtr, 1, uint32_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

// Activate the fake function with mock_call_use_as_fake(TO_STR(zpal_retention_register_write))
zpal_status_t zpal_retention_register_write_FAKE(uint32_t regIndex, uint32_t dataVal)
{
  if (regIndex >= RETENTION_REG_COUNT)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }
  mRetentionRegs[regIndex] = dataVal;

  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_retention_register_write(uint32_t regIndex, uint32_t dataVal)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(zpal_retention_register_write_FAKE, regIndex, dataVal);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);

  MOCK_CALL_ACTUAL(p_mock, regIndex, dataVal);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, regIndex);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, dataVal);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_retention_register_write_private(uint32_t regIndex, uint32_t dataVal)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(zpal_retention_register_write_FAKE, regIndex, dataVal);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);

  MOCK_CALL_ACTUAL(p_mock, regIndex, dataVal);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, regIndex);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, dataVal);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}
