// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_network_management_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZAF_network_management.h>
#include "mock_control.h"

#define MOCK_FILE "ZAF_network_management_mock.c"

void ZAF_SetMaxInclusionRequestIntervals(uint32_t intervals)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, intervals);

  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG0, intervals);
}
