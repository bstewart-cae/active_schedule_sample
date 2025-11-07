/**
 * @file
 * @copyright 2023 Silicon Laboratories Inc.
 */
#include "unity.h"
#include "mock_control.h"
#include "ZW_main_region.h"
#include "string.h"

static zpal_radio_profile_t rfProfile;

void setUpSuite(void)
{
}

void tearDownSuite(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

bool zpal_radio_get_long_range_channel_auto_mode(void)
{
  return 0;
}

void test_ProtocolLongRangeChannelSet(void)
{
  mock_t * pMock;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(StorageSetPrimaryLongRangeChannelId));

// test we can set LR channel to A when current channel is uninitialized
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_3;

  mock_call_expect(TO_STR(zpal_radio_get_primary_long_range_channel), &pMock);
  pMock->return_code.value = ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED;

  mock_call_expect(TO_STR(zpal_radio_set_primary_long_range_channel), &pMock);
  pMock->expect_arg[ARG0].v = ZPAL_RADIO_LR_CHANNEL_A;

  mock_call_expect(TO_STR(zpal_radio_get_rf_profile), &pMock);
  pMock->return_code.p = &rfProfile;

  mock_call_expect(TO_STR(ZW_LrChannelConfigToUse), &pMock);
  pMock->expect_arg[ARG0].p = &rfProfile;
  pMock->return_code.value = ZPAL_RADIO_LR_CH_CFG1;

  bool result = ProtocolLongRangeChannelSet(ZPAL_RADIO_LR_CHANNEL_A);
  TEST_ASSERT_EQUAL_MESSAGE(true, result, "Wrong default value 1");

  // test we can set LR channel to B when current channel is A

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_3;

  mock_call_expect(TO_STR(zpal_radio_get_primary_long_range_channel), &pMock);
  pMock->return_code.value = ZPAL_RADIO_LR_CHANNEL_A;

  mock_call_expect(TO_STR(zpal_radio_set_primary_long_range_channel), &pMock);
  pMock->expect_arg[ARG0].v = ZPAL_RADIO_LR_CHANNEL_B;

  mock_call_expect(TO_STR(zpal_radio_get_rf_profile), &pMock);
  pMock->return_code.p = &rfProfile;

  mock_call_expect(TO_STR(ZW_LrChannelConfigToUse), &pMock);
  pMock->expect_arg[ARG0].p = &rfProfile;
  pMock->return_code.value = ZPAL_RADIO_LR_CH_CFG1;

  result = ProtocolLongRangeChannelSet(ZPAL_RADIO_LR_CHANNEL_B);
  TEST_ASSERT_EQUAL_MESSAGE(true, result, "Wrong default value 2");

  // test we can't set LR channel to B when current channel is the same

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_3;

  mock_call_expect(TO_STR(zpal_radio_get_primary_long_range_channel), &pMock);
  pMock->return_code.value = ZPAL_RADIO_LR_CHANNEL_B;

  result = ProtocolLongRangeChannelSet(ZPAL_RADIO_LR_CHANNEL_B);
  TEST_ASSERT_EQUAL_MESSAGE(false, result, "Wrong default value 3");

// test LR channel is not set when region is not LR

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_2;

  result = ProtocolLongRangeChannelSet(ZPAL_RADIO_LR_CHANNEL_A);
  TEST_ASSERT_EQUAL_MESSAGE(false, result, "Wrong default value 4");
  mock_calls_verify();
}
