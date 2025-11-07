// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include <ZW_home_id_hash.h>
#include <zpal_radio_utils.h>
#include "mock_control.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

/*
 * Test HomeIdHashCalculate
 */
void test_HomeIdHashCalculate(void)
{
  mock_calls_clear();

  uint32_t homeId = 0xDEADBE78;
  uint32_t nodeId = 256;

  /*
   * Test 1 - HomeIdHashCalculate output for Long Range included node on REGION_US_LR
   *
   * Expected:
   * - Long Range have no Illegal HomIdHash values so no increment is done on the calculated value
   *
   */
  // RadioSetRegionSettings(REGION_US_LR);
  // zpal_radio_protocol_mode_t pmode = zpal_radio_get_protocol_mode();
  uint8_t homeIdHash = HomeIdHashCalculate(homeId, nodeId, zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1));
  TEST_ASSERT_EQUAL_MESSAGE(0x4A, homeIdHash, "HomeIdHash on REGION_US_LR on LR channel - Unexpected HomeIdHash returned!");

  /*
   * Test 2 - HomeIdHashCalculate output for Classic included node on REGION_US_LR
   *
   * Expected:
   * - 2 Channel region has the calculated value as an Illegal value so returned value should be incremented accordingly
   *
   */
  nodeId = 5;
  homeIdHash = HomeIdHashCalculate(homeId, nodeId, zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1));
  TEST_ASSERT_EQUAL_MESSAGE(0x4B, homeIdHash, "HomeIdHash on REGION_US_LR on Classic channel #1 - Unexpected HomeIdHash returned!");

  /*
   * Test 3 - HomeIdHashCalculate output for Classic included node on REGION_US_LR
   *
   * Expected:
   * - 2 Channel region do not have the calculated value as an Illegal value so no increment on the returned value
   *
   */
  homeId = 0xDEADBE17;
  homeIdHash = HomeIdHashCalculate(homeId, nodeId, zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1));
  TEST_ASSERT_EQUAL_MESSAGE(0x25, homeIdHash, "HomeIdHash on REGION_US_LR on Classic channel #2 - Unexpected HomeIdHash returned!");

  /*
   * Test 4 - HomeIdHashCalculate output for included node on REGION_KR
   *
   * Expected:
   * - 3 Channel region has the calculated value as an Illegal value so returned value should be incremented accordingly
   *
   */
  homeIdHash = HomeIdHashCalculate(homeId, nodeId, zpal_radio_region_get_protocol_mode(REGION_KR, ZPAL_RADIO_LR_CH_CFG_NO_LR));
  TEST_ASSERT_EQUAL_MESSAGE(0x26, homeIdHash, "HomeIdHash on REGION_KR - Unexpected HomeIdHash returned!");
  mock_calls_verify();
}
