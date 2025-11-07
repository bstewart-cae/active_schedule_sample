// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_slave_tx_power.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "ZW_dynamic_tx_power.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

void test_Save_Read_TxPowerAndRSSI(void)
{
  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  int8_t txPower = 0;
  int8_t rssi = 0;

  //Test negative inputs
  SaveTxPowerAndRSSI(-6, -110);
  ReadTxPowerAndRSSI(&txPower, &rssi);
  TEST_ASSERT_EQUAL(-6, txPower);
  TEST_ASSERT_EQUAL(-110, rssi);

  //Test positive inputs
  SaveTxPowerAndRSSI(10, 3);
  ReadTxPowerAndRSSI(&txPower, &rssi);
  TEST_ASSERT_EQUAL(10, txPower);
  TEST_ASSERT_EQUAL(3, rssi);

  mock_calls_verify();
}
