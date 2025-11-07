// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_home_id_generator_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_home_id_generator.h>
#include <string.h>
#include "mock_control.h"
#include <Assert.h>

#define MOCK_FILE "ZW_home_id_generator_mock.c"

uint32_t HomeIdGeneratorGetNewId(uint8_t * pHomeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0xC1234567);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, pHomeID);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pHomeID);

  if (NULL != pHomeID)
  {
    memcpy(pHomeID, pMock->output_arg[0].p, 4);
  }
  else
  {
    ASSERT(false);
  }

  MOCK_CALL_RETURN_VALUE(pMock, uint32_t);
}
