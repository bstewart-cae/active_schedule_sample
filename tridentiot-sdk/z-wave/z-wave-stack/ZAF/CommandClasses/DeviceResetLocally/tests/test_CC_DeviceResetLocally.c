/**
 * @file test_CC_DeviceResetLocally.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <CC_Common.h>
#include <mock_control.h>
#include <unity.h>
#include <CC_DeviceResetLocally.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

static bool callback_was_called = false;

void CC_DeviceResetLocally_done(transmission_result_t * pTransmissionResult)
{
  transmission_result_t transmissionResult;
  transmissionResult.nodeId = 0;
  transmissionResult.status = 0;
  transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(transmissionResult.nodeId, pTransmissionResult->nodeId, "A!");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(transmissionResult.status, pTransmissionResult->status, "B!");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(transmissionResult.isFinished, pTransmissionResult->isFinished, "C!");
  callback_was_called = true;
}

void test_CC_DeviceResetLocally_transmit_not_included(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  callback_was_called = false;

  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = 0;

  CC_DeviceResetLocally_notification_tx();

  TEST_ASSERT_TRUE(callback_was_called);

  mock_calls_verify();
}

void test_CC_DeviceResetLocally_transmit_tx_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  callback_was_called = false;

  agi_profile_t agi_profile = {
    0x00, 
    0x01
  };
  cc_group_t cmdGrpExpected = {
    0x5A,
    0x01
  };

  mock_call_use_as_stub(TO_STR(ZAF_GetNodeID));

  mock_call_expect(TO_STR(cc_engine_multicast_request), &pMock);
  pMock->expect_arg[0].p = &agi_profile;
  pMock->expect_arg[1].v = 0; // ENDPOINT_ROOT
  pMock->expect_arg[2].p = &cmdGrpExpected;
  pMock->compare_rule_arg[3] = COMPARE_NULL;
  pMock->expect_arg[4].v = 0;
  pMock->expect_arg[5].v = 0;
  pMock->expect_arg[6].p = CC_DeviceResetLocally_done;
  pMock->return_code.v = 0; // JOB_STATUS_SUCCESS

  CC_DeviceResetLocally_notification_tx();

  TEST_ASSERT_FALSE(callback_was_called);

  mock_calls_verify();
}
