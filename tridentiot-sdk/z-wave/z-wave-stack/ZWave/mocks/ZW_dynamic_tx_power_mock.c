// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_dynamic_tx_power_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_dynamic_tx_power_algorithm.h"
#include <stdint.h>
#include "mock_control.h"
#include "unity.h"

#define MOCK_FILE "ZW_dynamic_tx_power_mock.c"


int8_t ZW_DynamicTxPowerAlgorithm(int8_t txPower, int8_t RSSIValue, int8_t noisefloor, TX_POWER_RETRANSMISSION_TYPE retransmissionType)
{
  return 0;
}

void
SaveTxPowerAndRSSI(int8_t txPower, int8_t rssi)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, txPower, rssi);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, txPower);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, rssi);
}

void
ReadTxPowerAndRSSI(int8_t * txPower, int8_t * rssi)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, txPower, rssi);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, txPower);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, rssi);
}
