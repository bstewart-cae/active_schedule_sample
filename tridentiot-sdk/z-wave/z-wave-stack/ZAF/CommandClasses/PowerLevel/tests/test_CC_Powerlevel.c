/**
 * @file test_CC_Powerlevel.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <string.h>
#include <ZW_application_transport_interface.h>
#include <test_common.h>
#include <SwTimer.h>
#include <ZW_basis_api.h>
#include "ZW_TransportEndpoint.h"
#include "ZAF_CC_Invoker.h"
#include "QueueNotifying_mock.h"
#include "zaf_transport_tx_mock.h"

static SZwaveTransmitPackage FramePackage_CALLBACK;

EQueueNotifyingStatus QueueNotifyingSendToBack_CALLBACK(SQueueNotifying* pThis, const uint8_t* pItem, uint32_t iTimeToWait, int cmock_num_calls)
{
  // If pItem is set save it to FramePackage_CALLBACK
  // This allows the callback to be accessed from another scope
  if(pItem) {
    memmove(&FramePackage_CALLBACK, pItem, sizeof(SZwaveTransmitPackage));
  } else {
    memset(&FramePackage_CALLBACK, 0, sizeof(SZwaveTransmitPackage));
  }

  return EQUEUENOTIFYING_STATUS_SUCCESS;
}

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {
  // Add the callback for every test case
  QueueNotifyingSendToBack_AddCallback(QueueNotifyingSendToBack_CALLBACK);
}

void tearDown(void) {

}

#define CC_Powerlevel_handler(a,b,c,d,e) invoke_cc_handler_v2(a,b,c,d,e)

static SQueueNotifying txQueueNotifying_mock;

static command_handler_input_t * powerlevel_test_node_set_frame_create(
    node_id_t testNodeID,
    uint8_t   powerLevel,
    uint16_t  testFrameCount);

static command_handler_input_t * powerlevel_set_frame_create(
    uint8_t power_level,
    uint8_t timeout);

static command_handler_input_t * powerlevel_get_frame_create(void);

void test_POWERLEVEL_TEST_NODE_SET_test_frame_count_zero(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t frameCount = 0;
  uint8_t commandLength = 0;

  const node_id_t TEST_NODE_ID = 1;
  const uint16_t  TEST_FRAME_COUNT = 0;

  memset((uint8_t *)&frame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x73; // COMMAND_CLASS_POWERLEVEL
  pFrame[frameCount++] = 0x04; // POWERLEVEL_TEST_NODE_SET
  pFrame[frameCount++] = (uint8_t)TEST_NODE_ID;            // Works only for nodeIds below 255 (Z-Wave)
  pFrame[frameCount++] = 3;    // Power level
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT >> 8); // Test frame count MSB
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT);      // Test frame count LSB
  commandLength = frameCount;

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(&rxOptions, &frame, commandLength, &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  mock_calls_verify();
}

void test_POWERLEVEL_TEST_NODE_SET_test_frame_count_1_success(void)
{
  mock_t * pMock = NULL;
  mock_t * pMockAppTimerRegister = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t frameCount = 0;
  uint8_t commandLength = 0;

  const node_id_t TEST_NODE_ID = 1;
  const uint16_t  TEST_FRAME_COUNT = 1;
  const uint16_t  TEST_FRAME_COUNT_SUCCESS = 1;

  mock_call_expect(TO_STR(ZAF_getZwTxQueue), &pMock);
  pMock->return_code.p = &txQueueNotifying_mock;

  memset((uint8_t *)&frame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x73; // COMMAND_CLASS_POWERLEVEL
  pFrame[frameCount++] = 0x04; // POWERLEVEL_TEST_NODE_SET
  pFrame[frameCount++] = (uint8_t)TEST_NODE_ID;            // Works only for nodeIds below 255 (Z-Wave)
  const uint8_t POWER_LEVEL = 3;
  pFrame[frameCount++] = POWER_LEVEL;
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT >> 8); // Test frame count MSB
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT);      // Test frame count LSB
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_expect(TO_STR(AppTimerRegister), &pMockAppTimerRegister);
  pMockAppTimerRegister->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->expect_arg[1].v = false;
  pMockAppTimerRegister->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->return_code.v = true; // Timer handle

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 40; // 40 milliseconds timeout
  pMock->return_code.v = 0; // Timer handle

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(&rxOptions, &frame, commandLength, &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_TESTFRAME;
  FramePackage.uTransmitParams.Test.DestNodeId = TEST_NODE_ID;
  FramePackage.uTransmitParams.Test.PowerLevel = POWER_LEVEL;

  QueueNotifyingSendToBack_ExpectAndReturn(&txQueueNotifying_mock, (const uint8_t*) &FramePackage, 0, EQUEUENOTIFYING_STATUS_SUCCESS);

  // Calling the function given to AppTimerRegister
  void (* AppTimerRegister_callback)(void);
  AppTimerRegister_callback = pMockAppTimerRegister->actual_arg[2].p;
  AppTimerRegister_callback();

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  mock_call_use_as_stub(TO_STR(CommandClassSupervisionGetAdd));

  uint8_t expectedFrame[] = {
                             0x73, // COMMAND_CLASS_POWERLEVEL
                             0x06, // POWERLEVEL_TEST_NODE_REPORT
							 (uint8_t)TEST_NODE_ID,
                             0x01, // POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES
                             (uint8_t)(TEST_FRAME_COUNT_SUCCESS >> 8), // Test frame acknowledged count (MSB)
                             (uint8_t)(TEST_FRAME_COUNT_SUCCESS)       // Test frame acknowledged count (LSB)
  };

  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  transmission_result_t transmitResult1 = {
        .status = TRANSMIT_COMPLETE_OK,
        .isFinished = TRANSMISSION_RESULT_UNKNOWN
  };

  // Calling the function given to QueueNotifyingSendToBack
  FramePackage_CALLBACK.uTransmitParams.Test.Handle(&transmitResult1);

  mock_calls_verify();
}

void test_POWERLEVEL_TEST_NODE_SET_test_frame_count_2_success(void)
{
  mock_t * pMock = NULL;
  mock_t * pMockAppTimerRegister = NULL;

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t frameCount = 0;
  uint8_t commandLength = 0;

  const node_id_t TEST_NODE_ID = 1;
  const uint16_t  TEST_FRAME_COUNT = 2;
  const uint16_t  TEST_FRAME_COUNT_SUCCESS = 2;

  memset((uint8_t *)&frame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x73; // COMMAND_CLASS_POWERLEVEL
  pFrame[frameCount++] = 0x04; // POWERLEVEL_TEST_NODE_SET
  pFrame[frameCount++] = (uint8_t)TEST_NODE_ID;
  const uint8_t POWER_LEVEL = 3;
  pFrame[frameCount++] = POWER_LEVEL;
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT >> 8); // Test frame count MSB
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT);      // Test frame count LSB
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  void (* AppTimerRegister_callback)(void);

  mock_call_expect(TO_STR(AppTimerRegister), &pMockAppTimerRegister);
  pMockAppTimerRegister->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->expect_arg[1].v = false;
  pMockAppTimerRegister->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->return_code.v = true; // Timer handle

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 40; // 40 milliseconds timeout
  pMock->return_code.v = 0; // Timer handle

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(&rxOptions, &frame, commandLength, &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_TESTFRAME;
  FramePackage.uTransmitParams.Test.DestNodeId = TEST_NODE_ID;
  FramePackage.uTransmitParams.Test.PowerLevel = POWER_LEVEL;

  QueueNotifyingSendToBack_ExpectAndReturn(&txQueueNotifying_mock, (const uint8_t*) &FramePackage, 0, EQUEUENOTIFYING_STATUS_SUCCESS);

  mock_call_expect(TO_STR(ZAF_getZwTxQueue), &pMock);
  pMock->return_code.p = &txQueueNotifying_mock;

  // Calling the function given to AppTimerRegister
  AppTimerRegister_callback = pMockAppTimerRegister->actual_arg[2].p;
  AppTimerRegister_callback();

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 40;
  pMock->return_code.v = 0;

    transmission_result_t transmitResult2 = {
        .status = TRANSMIT_COMPLETE_OK,
        .isFinished = TRANSMISSION_RESULT_UNKNOWN
  };

  // Calling the function given to QueueNotifyingSendToBack
  FramePackage_CALLBACK.uTransmitParams.Test.Handle(&transmitResult2);

  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_TESTFRAME;
  FramePackage.uTransmitParams.Test.DestNodeId = TEST_NODE_ID;
  FramePackage.uTransmitParams.Test.PowerLevel = POWER_LEVEL;

  QueueNotifyingSendToBack_ExpectAndReturn(&txQueueNotifying_mock, (const uint8_t*) &FramePackage, 0, EQUEUENOTIFYING_STATUS_SUCCESS);

  mock_call_expect(TO_STR(ZAF_getZwTxQueue), &pMock);
  pMock->return_code.p = &txQueueNotifying_mock;

  // Calling the function given to AppTimerRegister
  AppTimerRegister_callback = pMockAppTimerRegister->actual_arg[2].p;
  AppTimerRegister_callback();


  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  mock_call_use_as_stub(TO_STR(CommandClassSupervisionGetAdd));

  uint8_t expectedFrame[] = {
                             0x73, // COMMAND_CLASS_POWERLEVEL
                             0x06, // POWERLEVEL_TEST_NODE_REPORT
							 (uint8_t)TEST_NODE_ID,
                             0x01, // POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES
                             (uint8_t)(TEST_FRAME_COUNT_SUCCESS >> 8), // Test frame acknowledged count (MSB)
                             (uint8_t)(TEST_FRAME_COUNT_SUCCESS)       // Test frame acknowledged count (LSB)
  };
  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  transmission_result_t transmitResult3 = {
        .status = TRANSMIT_COMPLETE_OK,
        .isFinished = TRANSMISSION_RESULT_UNKNOWN
  };

  // Calling the function given to QueueNotifyingSendToBack
  FramePackage_CALLBACK.uTransmitParams.Test.Handle(&transmitResult3);

  mock_calls_verify();
}

void test_POWERLEVEL_TEST_NODE_SET_test_frame_count_loop_10_success(void)
{
  mock_t * pMock = NULL;
  mock_t * pMockAppTimerRegister = NULL;

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t frameCount = 0;
  uint8_t commandLength = 0;

  uint8_t expectedFrame[50];
  size_t  expectedFrameLength = 0;

  const node_id_t TEST_NODE_ID = 1;
  const uint16_t  TEST_FRAME_COUNT = 10;
  const uint16_t  TEST_FRAME_COUNT_SUCCESS = 10;

  /*
   * SEND POWERLEVEL_TEST_NODE_SET
   */
  memset((uint8_t *)&frame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x73; // COMMAND_CLASS_POWERLEVEL
  pFrame[frameCount++] = 0x04; // POWERLEVEL_TEST_NODE_SET
  pFrame[frameCount++] = (uint8_t)TEST_NODE_ID;
  const uint8_t POWER_LEVEL = 3;
  pFrame[frameCount++] = POWER_LEVEL;
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT >> 8); // Test frame count MSB
  pFrame[frameCount++] = (uint8_t)(TEST_FRAME_COUNT); // Test frame count LSB
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  void (* AppTimerRegister_callback)(void);

  mock_call_expect(TO_STR(AppTimerRegister), &pMockAppTimerRegister);
  pMockAppTimerRegister->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->expect_arg[1].v = false;
  pMockAppTimerRegister->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->return_code.v = true; // Timer handle

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 40; // 40 milliseconds timeout
  pMock->return_code.v = 0;

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(&rxOptions, &frame, commandLength, &frameOut, &frameOutLength);
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_TESTFRAME;
  FramePackage.uTransmitParams.Test.DestNodeId = TEST_NODE_ID;
  FramePackage.uTransmitParams.Test.PowerLevel = POWER_LEVEL;

  uint8_t i;
  for (i = 0; i < TEST_FRAME_COUNT; i++)
  {
    QueueNotifyingSendToBack_ExpectAndReturn(&txQueueNotifying_mock, (const uint8_t*) &FramePackage, 0, EQUEUENOTIFYING_STATUS_SUCCESS);

    mock_call_expect(TO_STR(ZAF_getZwTxQueue), &pMock);
    pMock->return_code.p = &txQueueNotifying_mock;

    // Calling the function given to AppTimerRegister
    AppTimerRegister_callback = pMockAppTimerRegister->actual_arg[2].p;
    AppTimerRegister_callback();

    if (i < (TEST_FRAME_COUNT - 1))
    {
      mock_call_expect(TO_STR(TimerStop), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
      pMock->return_code.v = 0;

      mock_call_expect(TO_STR(TimerStart), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
      pMock->expect_arg[1].v = 40;
      pMock->return_code.v = 0;
    }
    else
    {
      mock_call_expect(TO_STR(TimerStop), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
      pMock->return_code.v = 0;

      mock_call_use_as_stub(TO_STR(CommandClassSupervisionGetAdd));

      expectedFrame[expectedFrameLength++] = 0x73; // COMMAND_CLASS_POWERLEVEL
      expectedFrame[expectedFrameLength++] = 0x06; // POWERLEVEL_TEST_NODE_REPORT
      expectedFrame[expectedFrameLength++] = (uint8_t)TEST_NODE_ID;
      expectedFrame[expectedFrameLength++] = 0x01; // POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES
      expectedFrame[expectedFrameLength++] = (uint8_t)(TEST_FRAME_COUNT_SUCCESS >> 8); // Test frame acknowledged count (MSB)
      expectedFrame[expectedFrameLength++] = (uint8_t)(TEST_FRAME_COUNT_SUCCESS);      // Test frame acknowledged count (LSB)

      zaf_transport_tx_ExpectAndReturn(expectedFrame, expectedFrameLength, NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_zaf_tx_options();
    }

    transmission_result_t transmitResult4 = {
        .status = TRANSMIT_COMPLETE_OK,
        .isFinished = TRANSMISSION_RESULT_UNKNOWN
    };

    // Calling the function given to QueueNotifyingSendToBack
    FramePackage_CALLBACK.uTransmitParams.Test.Handle(&transmitResult4);
  }

  /*
   * SEND POWERLEVEL_TEST_NODE_GET
   */
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x73; // COMMAND_CLASS_POWERLEVEL
  pFrame[frameCount++] = 0x05; // POWERLEVEL_TEST_NODE_GET
  commandLength = frameCount;

  status = CC_Powerlevel_handler(&rxOptions, &frame, commandLength, &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  expectedFrameLength,
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        expectedFrameLength,
                                        "Frame does not match");
                                        
  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  mock_calls_verify();
}

void test_POWERLEVEL_TEST_NODE_SET_already_in_progress(void)
{
  command_handler_input_t * p_chi = powerlevel_test_node_set_frame_create(
        10, // Don't care.
        5, // Don't care.
        10); // Don't care.

  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerStart));

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  // Call the handler once more to verify that it will not start another test.
  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_POWERLEVEL_SET_invalid_power_level(void)
{
  mock_calls_clear();

  // Create frame with invalid power level.
  command_handler_input_t * p_chi = powerlevel_set_frame_create(10, 5);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_POWERLEVEL_SET_normal_power_level(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();

  const uint8_t POWER_LEVEL = 0;

  // Create frame with normal power (where timeout is ignored cf. specification.
  command_handler_input_t * p_chi = powerlevel_set_frame_create(POWER_LEVEL, 5);

  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerStop));

  SApplicationHandles AppHandle;
  SQueueNotifying ZwCommandQueue = { 0 };
  AppHandle.pZwCommandQueue = &ZwCommandQueue;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &AppHandle;

  SZwaveCommandPackage cmdPackage = {0};
  cmdPackage.eCommandType = EZWAVECOMMANDTYPE_ZW_SET_TX_ATTENUATION;
  cmdPackage.uCommandParams.SetTxAttenuation.value = 0;

  QueueNotifyingSendToBack_ExpectAndReturn(AppHandle.pZwCommandQueue, (const uint8_t*) &cmdPackage, 500, EQUEUENOTIFYING_STATUS_SUCCESS);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_POWERLEVEL_SET_timeout_zero(void)
{
  mock_calls_clear();

  const uint8_t POWER_LEVEL = 5; // Don't care since we're setting timeout to zero.
  const uint8_t TIMEOUT = 0;

  // Create frame with normal power (where timeout is ignored cf. specification.)
  command_handler_input_t * p_chi = powerlevel_set_frame_create(POWER_LEVEL, TIMEOUT);

  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerStop));

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

CC_handler_map_latest_t const * get_cc_handle(void)
{
  CC_handler_map_latest_t const * handle = &cc_handlers_start;

  if (COMMAND_CLASS_POWERLEVEL != handle->CC)
  {
    ASSERT(false);
  }

  return handle;
}

void test_POWERLEVEL_SET_valid_power_level(void)
{
  mock_t * pMock = NULL;
  mock_t * pMockAppTimerRegister = NULL;
  //mock_t * pMockSendTestFrame = NULL;
  mock_calls_clear();

  const uint8_t POWER_LEVEL = 5; // Random, valid power level not being the normal level.
  const uint32_t TIMEOUT_SEC = 10;

  SApplicationHandles AppHandle;
  SQueueNotifying ZwCommandQueue = { 0 };
  AppHandle.pZwCommandQueue = &ZwCommandQueue;
  
  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &AppHandle;

  SZwaveCommandPackage cmdPackage = {0};
  cmdPackage.eCommandType = EZWAVECOMMANDTYPE_ZW_SET_TX_ATTENUATION;
  cmdPackage.uCommandParams.SetTxAttenuation.value = 0;

  QueueNotifyingSendToBack_ExpectAndReturn(AppHandle.pZwCommandQueue, (const uint8_t*) &cmdPackage, 500, EQUEUENOTIFYING_STATUS_SUCCESS);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  CC_handler_map_latest_t const * cc_handle = get_cc_handle();
  cc_handle->reset();

  command_handler_input_t * p_chi = powerlevel_set_frame_create(POWER_LEVEL, TIMEOUT_SEC);

  mock_call_expect(TO_STR(AppTimerRegister), &pMockAppTimerRegister);
  pMockAppTimerRegister->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->expect_arg[1].v = true;
  pMockAppTimerRegister->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockAppTimerRegister->return_code.v = true;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 1000; // 1000 milliseconds timeout
  pMock->return_code.v = 0;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &AppHandle;

  cmdPackage.eCommandType = EZWAVECOMMANDTYPE_ZW_SET_TX_ATTENUATION;
  cmdPackage.uCommandParams.SetTxAttenuation.value = 5;

  QueueNotifyingSendToBack_ExpectAndReturn(AppHandle.pZwCommandQueue, (const uint8_t*) &cmdPackage, 500, EQUEUENOTIFYING_STATUS_SUCCESS);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;
  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong SET frame status :(");

  test_common_command_handler_input_free(p_chi);

  /*
   * Transmit a Powerlevel Get command.
   */
  p_chi = powerlevel_get_frame_create();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(GetResponseBuffer));

  const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_POWERLEVEL,
                              POWERLEVEL_REPORT,
                              POWER_LEVEL,
                              TIMEOUT_SEC
  };

  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(EXPECTED_FRAME),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(EXPECTED_FRAME, &frameOut,
                                        sizeof(EXPECTED_FRAME),
                                        "Frame does not match");

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong GET frame status :(");

  /*
   * Setup mocks and call timer callback to stop the test.
   */
  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  pMock->return_code.p = &AppHandle;

  cmdPackage.eCommandType = EZWAVECOMMANDTYPE_ZW_SET_TX_ATTENUATION;
  cmdPackage.uCommandParams.SetTxAttenuation.value = 0;

  QueueNotifyingSendToBack_ExpectAndReturn(AppHandle.pZwCommandQueue, (const uint8_t*) &cmdPackage, 500, EQUEUENOTIFYING_STATUS_SUCCESS);

  // Call the function given to AppTimerRegister TIMEOUT_SEC times
  void (* AppTimerRegister_callback)(SSwTimer*);
  AppTimerRegister_callback = pMockAppTimerRegister->actual_arg[2].p;
  SSwTimer * timer = pMockAppTimerRegister->actual_arg[0].p;
  for (uint8_t i=0; i < TIMEOUT_SEC; i++)
  {
    AppTimerRegister_callback(timer);
  }

  /*
   * Transmit a 2nd Powerlevel Get command and expect the power level
   * to be back at norrmalPower (0) and the timeout value to have counted
   * down to 0 as well
   */
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(GetResponseBuffer));

  const uint8_t EXPECTED_FRAME_2[] = {
                              COMMAND_CLASS_POWERLEVEL,
                              POWERLEVEL_REPORT,
                              0, // Power level expected back at normalPower (0)
                              0  // Timeout seconds expected to have counted down to 0
  };

  status = CC_Powerlevel_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(EXPECTED_FRAME_2),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(EXPECTED_FRAME_2, &frameOut,
                                        sizeof(EXPECTED_FRAME_2),
                                        "Frame does not match");

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong 2nd GET frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

static command_handler_input_t * powerlevel_test_node_set_frame_create(
    node_id_t testNodeID,
    uint8_t   powerLevel,
    uint16_t  testFrameCount)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_POWERLEVEL;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = POWERLEVEL_TEST_NODE_SET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)testNodeID;     // Insert LSB for Z-Wave (not Z-Wave LR)
  p_chi->frame.as_byte_array[p_chi->frameLength++] = powerLevel;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(testFrameCount >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(testFrameCount);
  return p_chi;
}

static command_handler_input_t * powerlevel_set_frame_create(
    uint8_t power_level,
    uint8_t timeout)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_POWERLEVEL;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = POWERLEVEL_SET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = power_level;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = timeout;
  return p_chi;
}

static command_handler_input_t * powerlevel_get_frame_create(void)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_POWERLEVEL;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = POWERLEVEL_GET;
  return p_chi;
}
