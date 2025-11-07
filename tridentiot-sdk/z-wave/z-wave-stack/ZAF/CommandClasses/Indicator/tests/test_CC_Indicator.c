/**
 * @file test_CC_Indicator.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <test_common.h>
#include <string.h>
#include <mock_control.h>
#include <ZW_TransportLayer.h>
#include <CC_Common.h>
#include "CC_Indicator.h"
#include "CC_IndicatorPrivate.h"
#include "ZW_TransportEndpoint.h"
#include <ZAF_TSE.h>
#include "ZAF_CC_Invoker.h"
#include "zaf_transport_tx_mock.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

#define MOCK_FILE "test_CC_Indicator.c"

/****************************************************************************/
/*                              TEST CASES                                  */
/****************************************************************************/

/**
 * Test the version one set/get commands
 */
void test_INDICATOR_SET_GET_V1_WITH_TSE(void)
{
  mock_t*                         pMock  = NULL;
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;
  zaf_tx_options_t                txOptions;
  s_CC_indicator_data_t           indicator_data;

  //----------------------------------------------------------
  // Turn on the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_on[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0xFF                            // Indicator0 value
  };

  mock_calls_clear();

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;       //Overwrite_previous_trigger
  pMock->return_code.v = true;

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_on,
                          sizeof(cmd_frame_on), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  // Fetch the callback passed to ZAF_TSE_Trigger().
  zaf_tse_callback_t cb = (zaf_tse_callback_t)pMock->actual_arg[0].p;
  uint8_t expectedFrame[] = { COMMAND_CLASS_INDICATOR_V3, INDICATOR_REPORT_V3, 0xFF, 1, INDICATOR_REPORT_NA_V3, 0, 0 };
  
  memset((uint8_t*)&txOptions, 0x00, sizeof(zaf_tx_options_t));
  memset((uint8_t*)&indicator_data, 0x00, sizeof(s_CC_indicator_data_t));
  indicator_data.indicatorId = 0xFF;
  zaf_transport_tx_ExpectAndReturn((uint8_t *) &expectedFrame, sizeof(expectedFrame), ZAF_TSE_TXCallback, &txOptions, true);
  cb(&txOptions, &indicator_data);  

  mock_calls_verify();

  //----------------------------------------------------------
  // Get status of the indicator (ON)
  //----------------------------------------------------------

  uint8_t cmd_frame_get[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_GET_V3
  };

  uint8_t exp_frame_report_on[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_REPORT,
      0xFF               // Indicator0 value
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get,
                          sizeof(cmd_frame_get), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report_on),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report_on, &frameOut,
                                        sizeof(exp_frame_report_on),
                                        "Frame does not match");

  mock_calls_verify();

  //----------------------------------------------------------
  // Turn off the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_off[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0x00                            // Indicator0 value
  };

  mock_calls_clear();

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;       //Overwrite_previous_trigger
  pMock->return_code.v = true;

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_off,
                          sizeof(cmd_frame_off), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();

  //----------------------------------------------------------
  // Get status of the indicator (OFF)
  //----------------------------------------------------------

  uint8_t exp_frame_report_off[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_REPORT,
      0x00               // Indicator0 value
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get,
                          sizeof(cmd_frame_get), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report_off),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report_off, &frameOut,
                                        sizeof(exp_frame_report_off),
                                        "Frame does not match");


  mock_calls_verify();
}


/**
 * Test the version three set/get commands
 */
void test_INDICATOR_SET_GET_V3(void)
{
  mock_t*                         pMock  = NULL;
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Blink the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_blink[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0x00,                            // Indicator0 value (ignore)
      0x03,                            // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_OFF_PERIOD,
      0x0A,                            // Period length = 10 * 0.1 second
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_OFF_CYCLES,
      0x02,                            // 2 cycles
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_TIME,
      0x04                             // On time = 4 * 0.1 second
  };

  mock_calls_clear();

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;       //Overwrite_previous_trigger
  pMock->return_code.v = true;

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_blink,
                          sizeof(cmd_frame_blink), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();

  //----------------------------------------------------------
  // Get status of the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_get[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_GET_V3,
      INDICATOR_IND_NODE_IDENTIFY
  };

  uint8_t exp_frame_report[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_REPORT,
      0xFF,                          // Indicator0 value
      0x03,                          // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_OFF_PERIOD,
      0x0A,                          // Period length = 10 * 0.1 second
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_OFF_CYCLES,
      0x02,                          // 2 cycles
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_TIME,
      0x04                           // On time = 4 * 0.1 second
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get,
                          sizeof(cmd_frame_get), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report, &frameOut,
                                        sizeof(exp_frame_report),
                                        "Frame does not match");

  mock_calls_verify();
}


/**
 * Test the version three set command with incomplete set of properties
 */
void test_INDICATOR_SET_V3_missing_prop(void)
{
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Blink the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_blink[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0x00,                            // Indicator0 value (ignore)
      0x02,                            // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_OFF_PERIOD,
      0x06,                            // Period length = 6 * 0.1 second
      /* INDICATOR_PROP_ON_OFF_CYCLES is required when we have
       * specified the on_off period. Leaving it out is an error.
          INDICATOR_IND_NODE_IDENTIFY,
          INDICATOR_PROP_ON_OFF_CYCLES,
          0x02,
      */
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_TIME,
      0x00                             // Symmetrical on/off
  };

  mock_calls_clear();

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_blink,
                          sizeof(cmd_frame_blink), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();

  //----------------------------------------------------------
  // Get status of the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_get[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_GET_V3,
      INDICATOR_IND_NODE_IDENTIFY
  };

  uint8_t exp_frame_report[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_REPORT,
      0xFF,                          // Indicator0 value
      0x03,                          // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_OFF_PERIOD,
      /* Since the set command failed the period length should
       * still be 0x0A as specified with the previous successful
       * set command.
       */
      0x0A,                          // Period length = 10 * 0.1 second
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_OFF_CYCLES,
      0x02,                          // 2 cycles
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_TIME,
      /* Since the set command failed on time should still be 0x04
       * as specified with the previous successful set command.
       */
      0x04                           // On time = 4 * 0.1 second
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get,
                          sizeof(cmd_frame_get), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report, &frameOut,
                                        sizeof(exp_frame_report),
                                        "Frame does not match");

  mock_calls_verify();
}


/**
 * Test the version three set/get commands with an invalid on time
 */
void test_INDICATOR_SET_GET_V3_invalid_on_time(void)
{
  mock_t*                         pMock  = NULL;
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Blink the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_blink[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0x00,                            // Indicator0 value (ignore)
      0x03,                            // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_OFF_PERIOD,
      0x08,                            // Period length = 8 * 0.1 second
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_OFF_CYCLES,
      0x04,                            // 4 cycles
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_TIME,
      0x0A                             // Invalid value (larger than ON_OFF_PERIOD)
  };

  mock_calls_clear();

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;       //Overwrite_previous_trigger
  pMock->return_code.v = true;

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_blink,
                          sizeof(cmd_frame_blink), &frameOut, &frameOutLength);

  /* The return value should indicate ON_TIME was invalid, even though
   * the indicator has been updated
   */
  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();

  //----------------------------------------------------------
  // Get status of the indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_get[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_GET_V3,
      INDICATOR_IND_NODE_IDENTIFY
  };

  uint8_t exp_frame_report[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_REPORT,
      0xFF,                          // Indicator0 value
      0x03,                          // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_OFF_PERIOD,
      0x08,                          // Period length = 8 * 0.1 second
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_OFF_CYCLES,
      0x04,                          // 4 cycles
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      INDICATOR_PROP_ON_TIME,
      0x00                           // Symmetrical on/off
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get,
                          sizeof(cmd_frame_get), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report, &frameOut,
                                        sizeof(exp_frame_report),
                                        "Frame does not match");

  mock_calls_verify();
}




/**
 * Test the version three Supported Get command
 */
void test_INDICATOR_SUPPORTED_GET_V3(void)
{
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Discover the available indicators
  //----------------------------------------------------------

  uint8_t cmd_frame_get_supported_all[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SUPPORTED_GET_V3,
      0x00                           // Indicator ID  0x00 = discovery
  };

  uint8_t propertySupportedBitMask = (1 << INDICATOR_PROP_ON_OFF_PERIOD) | (1 << INDICATOR_PROP_ON_OFF_CYCLES) | (1 << INDICATOR_PROP_ON_TIME);

  uint8_t exp_frame_report_supported_all[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SUPPORTED_REPORT_V3,
      INDICATOR_IND_NODE_IDENTIFY,   // Indicator ID
      0x00,                          // Next Indicator ID
      0x01,                          // Reserved:3 BitmaskLen:5
      propertySupportedBitMask
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get_supported_all,
                          sizeof(cmd_frame_get_supported_all), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report_supported_all),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report_supported_all, &frameOut,
                                        sizeof(exp_frame_report_supported_all),
                                        "Frame does not match");

  mock_calls_verify();

  //----------------------------------------------------------
  // Query the 'Node Identify' indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_get_supported_50[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SUPPORTED_GET_V3,
      INDICATOR_IND_NODE_IDENTIFY            // Indicator ID
  };

  uint8_t exp_frame_report_supported_50[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SUPPORTED_REPORT_V3,
      INDICATOR_IND_NODE_IDENTIFY,           // Indicator ID
      0x00,                                  // Next Indicator ID
      0x01,                                  // Reserved:3 BitmaskLen:5
      (1 << INDICATOR_PROP_ON_OFF_PERIOD) |  // Bitmask
      (1 << INDICATOR_PROP_ON_OFF_CYCLES) |
      (1 << INDICATOR_PROP_ON_TIME)
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get_supported_50,
                          sizeof(cmd_frame_get_supported_50), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report_supported_50),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report_supported_50, &frameOut,
                                        sizeof(exp_frame_report_supported_50),
                                        "Frame does not match");

  mock_calls_verify();

  //----------------------------------------------------------
  // Query unknown indicator
  //----------------------------------------------------------

  uint8_t cmd_frame_get_supported_nn[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SUPPORTED_GET_V3,
      0x01             // Indicator ID (unknown)
  };

  uint8_t exp_frame_report_supported_nn[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SUPPORTED_REPORT_V3,
      0x00,           // Indicator ID (invalid)
      0x00,           // Next Indicator ID
      0x00            // Reserved:3 BitmaskLen:5
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get_supported_nn,
                          sizeof(cmd_frame_get_supported_nn), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report_supported_nn),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report_supported_nn, &frameOut,
                                        sizeof(exp_frame_report_supported_nn),
                                        "Frame does not match");

  mock_calls_verify();
}


/**
 * Test the version one set command with invalid value
 */
void test_INDICATOR_SET_V1_invalid_value(void)
{
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Send invalid value to indicator (valid range: 0x00..0x63, 0xFF)
  //----------------------------------------------------------

  uint8_t cmd_frame_on[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0xEE                            // Indicator0 value
  };

  mock_calls_clear();

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_on,
                          sizeof(cmd_frame_on), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();
}


/**
 * Test handling of invalid command
 */
void test_INDICATOR_unknown_cmd(void)
{
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Send unknown command
  //----------------------------------------------------------

  uint8_t cmd_frame_unknown_cmd[] = {
      COMMAND_CLASS_INDICATOR_V3,
      0xEE,                             // Unknown command
      INDICATOR_IND_NODE_IDENTIFY
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_unknown_cmd,
                          sizeof(cmd_frame_unknown_cmd), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();
}


/**
 * Test getting value of unknown indicator
 */
void test_INDICATOR_GET_unknown_indicator_id(void)
{
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Get value of unknown indicator ID
  //----------------------------------------------------------

  uint8_t cmd_frame_get_unknown_ind[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_GET_V3,
      0x01                      // Unknown indicator ID
  };

  uint8_t exp_frame_report_ind0[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_REPORT,
      0xFF,                     // Indicator0 value
      0x01,                     // Reserved:3 ObjectCount:5
      INDICATOR_REPORT_NA_V3,   // Dummy indicator ID
      0x00,
      0x00
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_get_unknown_ind,
                          sizeof(cmd_frame_get_unknown_ind), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, rc,
                                  "Return value from invoke_cc_handler_v2()");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(exp_frame_report_ind0),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(exp_frame_report_ind0, &frameOut,
                                        sizeof(exp_frame_report_ind0),
                                        "Frame does not match");

  mock_calls_verify();
}


/**
 * Test handling of incomplete/truncated frame
 */
void test_INDICATOR_SET_truncated_frame(void)
{
  RECEIVE_OPTIONS_TYPE_EX         rx_opt = {0};
  received_frame_status_t         rc = RECEIVED_FRAME_STATUS_SUCCESS;
  ZW_APPLICATION_TX_BUFFER        frameOut;
  uint8_t                         frameOutLength;

  //----------------------------------------------------------
  // Send truncated frame
  //----------------------------------------------------------

  uint8_t cmd_frame_incomplete[] = {
      COMMAND_CLASS_INDICATOR_V3,
      INDICATOR_SET_V3,
      0x00,
      0x03,                          // Reserved:3 ObjectCount:5
      INDICATOR_IND_NODE_IDENTIFY,
      INDICATOR_PROP_ON_OFF_PERIOD
      // Truncated frame!!
  };

  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  rc = invoke_cc_handler_v2(&rx_opt,
                          (ZW_APPLICATION_TX_BUFFER*) cmd_frame_incomplete,
                          sizeof(cmd_frame_incomplete), &frameOut, &frameOutLength);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, rc,
                                  "Return value from invoke_cc_handler_v2()");

  mock_calls_verify();
}
