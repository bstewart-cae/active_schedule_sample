// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file Test_zpal_radio_utils.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "unity.h"
#include "mock_control.h"
#include <zpal_radio_utils.h>

typedef struct {
  uint32_t region;
  uint32_t channel_config;
  zpal_radio_protocol_mode_t expected;
}test_zpal_radio_region_get_protocol_mode_t;

const test_zpal_radio_region_get_protocol_mode_t test_vectors[] =
{
  //----- test normal beahvior -----
  // test all the 2 channel regions
  {.region=0,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=1,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=2,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=3,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=5,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=6,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=7,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=8,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  // test all the 3 channel regions
  {.region=32,  .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_2},
  {.region=33,  .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_2},
  // Test all LR regions
  {.region=9,   .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=9,   .channel_config=1,      .expected=ZPAL_RADIO_PROTOCOL_MODE_3},
  {.region=9,   .channel_config=2,      .expected=ZPAL_RADIO_PROTOCOL_MODE_3},
  {.region=9,   .channel_config=3,      .expected=ZPAL_RADIO_PROTOCOL_MODE_4},
  {.region=11,  .channel_config=0,      .expected=ZPAL_RADIO_PROTOCOL_MODE_1},
  {.region=11,  .channel_config=1,      .expected=ZPAL_RADIO_PROTOCOL_MODE_3},
  {.region=11,  .channel_config=2,      .expected=ZPAL_RADIO_PROTOCOL_MODE_3},
  {.region=11,  .channel_config=3,      .expected=ZPAL_RADIO_PROTOCOL_MODE_4},
};

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void test_zpal_radio_region_get_protocol_mode(void)
{
  uint32_t i;
  uint32_t testCount = sizeof(test_vectors)/sizeof(test_vectors[0]);
  zpal_radio_protocol_mode_t protocolmode;

  for (i = 0; i < testCount; i++)
  {
    protocolmode = zpal_radio_region_get_protocol_mode(test_vectors[i].region, test_vectors[i].channel_config);
    UNITY_TEST_ASSERT_EQUAL_INT(test_vectors[i].expected, protocolmode, i, "zpal_radio_region_get_protocol_mode test fail");
  }
}
