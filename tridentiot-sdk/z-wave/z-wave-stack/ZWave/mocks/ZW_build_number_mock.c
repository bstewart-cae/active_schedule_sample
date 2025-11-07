// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_build_number_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "unity.h"

#define MOCK_FILE "ZW_build_number_mock.c"

uint16_t ZW_GetProtocolBuildNumber(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint16_t);
}
