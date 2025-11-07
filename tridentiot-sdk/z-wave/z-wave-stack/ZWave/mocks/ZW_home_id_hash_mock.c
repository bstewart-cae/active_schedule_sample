// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include <ZW_home_id_hash.h>
#include "mock_control.h"

#define MOCK_FILE "ZW_home_id_hash_mock.c"

uint8_t HomeIdHashCalculate(uint32_t homeId, node_id_t nodeId, zpal_radio_protocol_mode_t zwProtMode)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x55);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_ACTUAL(p_mock, homeId, nodeId);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, homeId);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, nodeId);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, zwProtMode);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}
