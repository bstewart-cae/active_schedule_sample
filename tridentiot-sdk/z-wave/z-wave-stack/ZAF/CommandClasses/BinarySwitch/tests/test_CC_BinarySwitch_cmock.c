#include <unity.h>
#include <CC_BinarySwitch.h>
#include <string.h>
#include <test_common.h>
#include "ZAF_CC_Invoker.h"
#include "ZW_TransportEndpoint_mock.h"
#include "ZAF_TSE_mock.h"
#include "CC_BinarySwitch_config_mock.h"
#include "SizeOf.h"
#include "ZAF_Actuator_mock.h"
#include "CC_Supervision_mock.h"
#include "zaf_config_api_mock.h"
#include "zaf_transport_tx_mock.h"

/**
 * unity helper function that is called before the test execution
 */
void setUpSuite(void)
{
}

/**
 * unity helper function that is called after the test execution
 */
void tearDownSuite(void)
{
}

void setUp(void)
{

}

void tearDown(void)
{

}

void test_SWITCH_BINARY_GET(void)
{
  static cc_binary_switch_t binary_switches[] = {
    {
      .endpoint = 0,
      .default_duration = 60,
      .callback = NULL
    }
  };
  cc_binary_switch_get_config_ExpectAndReturn(binary_switches);
  cc_binary_switch_get_config_length_ExpectAndReturn(sizeof_array(binary_switches));

  ZAF_Actuator_Init_Expect(&binary_switches[0].actuator, 0, 99, 20, binary_switches[0].default_duration, NULL);
  ZAF_Actuator_Init_IgnoreArg_cc_callback();

  ZAF_Actuator_GetCurrentValue_ExpectAndReturn(&binary_switches[0].actuator, 0);

  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_BINARY);

  RECEIVE_OPTIONS_TYPE_EX rxOptions = {0};

  uint8_t cmdFrame[] = {
    COMMAND_CLASS_SWITCH_BINARY,
    SWITCH_BINARY_GET
  };

  Check_not_legal_response_job_IgnoreAndReturn(false);
  zaf_config_get_default_endpoint_IgnoreAndReturn(0);
  /*
   * 99 is used as a test target value to distinguish from 0 as the current value. The target value
   * would not be different from the current value on boot.
   */
  ZAF_Actuator_GetTargetValue_ExpectAndReturn(&binary_switches[0].actuator, 99);
  ZAF_Actuator_GetDurationRemaining_ExpectAndReturn(&binary_switches[0].actuator, 3);

  cc_handler_input_t input = {
      .rx_options = &rxOptions,
      .frame = (ZW_APPLICATION_TX_BUFFER *) cmdFrame,
      .length = sizeof(cmdFrame)
  };

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };
  received_frame_status_t res = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_SUCCESS, res);

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_SWITCH_BINARY,
                                     SWITCH_BINARY_REPORT,
                                     0,
                                     255, // 99 gets converted to 255
                                     3
  };

  TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_REPORT), output.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_REPORT, &frame_out, output.length);
}

// Used for caching the actual actuator callback passed to ZAF_Actuator_Init().
static zaf_actuator_callback_t actual_actuator_callback;

/*
 * This CMock callback saves the Actuator callback when ZAF_Actuator_Init() is invoked.
 */
void ZAF_Actuator_Init_stub(__attribute__((unused)) s_Actuator* pActuator,
                            __attribute__((unused)) uint8_t minValue,
                            __attribute__((unused)) uint8_t maxValue,
                            __attribute__((unused)) uint16_t refreshRate,
                            __attribute__((unused)) uint8_t durationDefault,
                            zaf_actuator_callback_t cc_callback,
                            __attribute__((unused)) int cmock_num_calls)
{
  actual_actuator_callback = cc_callback;
}

// Used for caching the actual callback passed to ZAF_TSE_Trigger().
zaf_tse_callback_t actual_ZAF_TSE_Trigger_callback;

/*
 * Caches the actual callback passed to ZAF_TSE_Trigger().
 */
bool ZAF_TSE_Trigger_stub(zaf_tse_callback_t pCallback,
                          __attribute__((unused)) void* pData,
                          __attribute__((unused)) bool overwrite_previous_trigger,
                          __attribute__((unused)) int cmock_num_calls)
{
  actual_ZAF_TSE_Trigger_callback = pCallback;
  return true;
}

static cc_binary_switch_t * p_actual_switch;

void switch_cb(cc_binary_switch_t * p_switch)
{
  p_actual_switch = p_switch;
}

/*
 * Verifies the flow of Binary Switch Set V2 with True Status and Supervision.
 *
 * Includes invoking the function passed to TSE.
 */
void test_SWITCH_BINARY_SET_V2_with_TSE_and_Supervision(void)
{
  static cc_binary_switch_t binary_switches[] = {
    {
      .endpoint = 0,
      .default_duration = 60,
      .callback = switch_cb
    }
  };
  cc_binary_switch_get_config_ExpectAndReturn(binary_switches);
  cc_binary_switch_get_config_length_ExpectAndReturn(sizeof_array(binary_switches));

  // Use as stub and ignore arguments as they are verified in test_SWITCH_BINARY_GET().
  ZAF_Actuator_Init_Stub(ZAF_Actuator_Init_stub);
  ZAF_Actuator_GetCurrentValue_IgnoreAndReturn(0);

  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_BINARY);

  RECEIVE_OPTIONS_TYPE_EX rxOptions = {0};
  rxOptions.bSupervisionActive      = true; // Frame was received with Supervision.
  rxOptions.statusUpdate            = true; // Supervision Get has status updated enabled.
  const uint8_t SESSION_ID          = 10;
  rxOptions.sessionId               = SESSION_ID;

  const uint8_t VALUE = 50; // "On"
  const uint8_t DURATION = 5;

  uint8_t cmdFrame[] = {
    COMMAND_CLASS_SWITCH_BINARY,
    SWITCH_BINARY_SET,
    VALUE,
    DURATION
  };

  const uint8_t LAST_ON_VALUE = 99;

  //ZAF_Actuator_GetLastOnValue_ExpectAndReturn(&binary_switches[0].actuator, LAST_ON_VALUE);
  ZAF_Actuator_Set_ExpectAndReturn(&binary_switches[0].actuator, VALUE, DURATION, EACTUATOR_NOT_CHANGING);

  cc_handler_input_t input = {
      .rx_options = &rxOptions,
      .frame = (ZW_APPLICATION_TX_BUFFER *) cmdFrame,
      .length = sizeof(cmdFrame)
  };

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };
  received_frame_status_t res = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_SUCCESS, res);
  TEST_ASSERT_EQUAL_UINT8(0, output.length);

  ZAF_Actuator_GetCurrentValue_ExpectAndReturn(&binary_switches[0].actuator, LAST_ON_VALUE);
  ZAF_Actuator_GetTargetValue_ExpectAndReturn(&binary_switches[0].actuator, LAST_ON_VALUE);

  p_actual_switch = NULL;
  actual_ZAF_TSE_Trigger_callback = NULL;

  /*
   * Set expectations to check arguments and then add a callback that caches the TSE trigger
   * callback.
   */
  ZAF_TSE_Trigger_ExpectAndReturn(NULL, &binary_switches[0], true, true);
  ZAF_TSE_Trigger_IgnoreArg_pCallback();
  ZAF_TSE_Trigger_AddCallback(ZAF_TSE_Trigger_stub);

  is_multicast_ExpectAndReturn(&binary_switches[0].rxOpt, false);

  zaf_transport_rx_to_tx_options_Expect(&binary_switches[0].rxOpt, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options(); // Ignore this argument

  CmdClassSupervisionReportSend_ExpectAndReturn(NULL, SESSION_ID, CC_SUPERVISION_STATUS_SUCCESS, 0, JOB_STATUS_SUCCESS);
  CmdClassSupervisionReportSend_IgnoreArg_tx_options();

  actual_actuator_callback(&binary_switches[0].actuator);

  // Verify that the switch callback was invoked with the right switch object.
  TEST_ASSERT_EQUAL_PTR(&binary_switches[0], p_actual_switch);

  // Verify that the callback passed to ZAF_TSE_Trigger() was different from NULL.
  TEST_ASSERT_NOT_NULL(actual_ZAF_TSE_Trigger_callback);

  /*
   * Invoke the function passed to TSE to verify that it works as expected.
   *
   * At this point current value and target value are the same as TSE is triggered at transition
   * end.
   */
  ZAF_Actuator_GetCurrentValue_ExpectAndReturn(&binary_switches[0].actuator, VALUE);
  ZAF_Actuator_GetTargetValue_ExpectAndReturn(&binary_switches[0].actuator, VALUE);
  ZAF_Actuator_GetDurationRemaining_ExpectAndReturn(&binary_switches[0].actuator, 0);

  uint8_t EXPECTED_FRAME[] = {
                                        COMMAND_CLASS_SWITCH_BINARY_V2,
                                        SWITCH_BINARY_REPORT_V2,
                                        255,
                                        255,
                                        0
  };

  zaf_tx_options_t tse_tx_options = {
    .source_endpoint = 0xAA
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), ZAF_TSE_TXCallback, &tse_tx_options, true);

  actual_ZAF_TSE_Trigger_callback(&tse_tx_options, &binary_switches[0]);
}

/*
 * Verifies that Binary Switch Report only reports 0x00 or 0xFF.
 */
void test_binary_switch_report(void)
{
  static cc_binary_switch_t binary_switches[] = {
    {
      .endpoint = 0,
      .default_duration = 60,
      .callback = switch_cb
    }
  };
  cc_binary_switch_get_config_ExpectAndReturn(binary_switches);
  cc_binary_switch_get_config_length_ExpectAndReturn(sizeof_array(binary_switches));

  // Use as stub and ignore arguments as they are verified in test_SWITCH_BINARY_GET().
  ZAF_Actuator_Init_Stub(ZAF_Actuator_Init_stub);
  ZAF_Actuator_GetCurrentValue_IgnoreAndReturn(0);

  /*
   * Initialize CC Binary Switch
   */
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_BINARY);

  /*
   * Transmit a Binary Switch Set with "on" (50)
   */
  RECEIVE_OPTIONS_TYPE_EX rxOptions = {0};

  const uint8_t VALUE = 50;
  const uint8_t DURATION = 0;

  uint8_t cmdFrame[] = {
    COMMAND_CLASS_SWITCH_BINARY,
    SWITCH_BINARY_SET,
    VALUE, // Interpreted as "on"
    DURATION
  };

  ZAF_Actuator_Set_ExpectAndReturn(&binary_switches[0].actuator, VALUE, DURATION, EACTUATOR_NOT_CHANGING);

  cc_handler_input_t input = {
      .rx_options = &rxOptions,
      .frame = (ZW_APPLICATION_TX_BUFFER *) cmdFrame,
      .length = sizeof(cmdFrame)
  };

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };
  received_frame_status_t res = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_SUCCESS, res);
  TEST_ASSERT_EQUAL_UINT8(0, output.length);

  /*
   * Invoke actuator callback
   */
  ZAF_Actuator_GetCurrentValue_StopIgnore();
  ZAF_Actuator_GetCurrentValue_ExpectAndReturn(&binary_switches[0].actuator, VALUE);
  ZAF_Actuator_GetTargetValue_ExpectAndReturn(&binary_switches[0].actuator, VALUE);
  ZAF_TSE_Trigger_IgnoreAndReturn(true);

  actual_actuator_callback(&binary_switches[0].actuator);

  /*
   * Transmit a Binary Switch Get and expect a Binary Switch Report with "on" (255)
   */
  uint8_t binary_switch_get[] = {
    COMMAND_CLASS_SWITCH_BINARY,
    SWITCH_BINARY_GET
  };

  memset(&frame_out, 0, sizeof(frame_out));

  const uint8_t EXPECTED_FRAME_OUT[] = {
                                        COMMAND_CLASS_SWITCH_BINARY_V2,
                                        SWITCH_BINARY_REPORT_V2,
                                        255,
                                        255,
                                        0
  };
  const uint8_t EXPECTED_FRAME_OUT_LENGTH = 5;

  //ZAF_Actuator_Set_ExpectAndReturn(&binary_switches[0].actuator, VALUE, DURATION, EACTUATOR_NOT_CHANGING);
  ZAF_Actuator_GetTargetValue_ExpectAndReturn(NULL, VALUE);
  ZAF_Actuator_GetTargetValue_IgnoreArg_pActuator();

  ZAF_Actuator_GetDurationRemaining_ExpectAndReturn(NULL, 0);
  ZAF_Actuator_GetDurationRemaining_IgnoreArg_pActuator();

  input.frame = (ZW_APPLICATION_TX_BUFFER *) binary_switch_get;
  input.length = sizeof(binary_switch_get);

  res = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_SUCCESS, res);
  TEST_ASSERT_EQUAL_UINT8(EXPECTED_FRAME_OUT_LENGTH, output.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(EXPECTED_FRAME_OUT, &frame_out, output.length, "Binary Switch Report didn't match expectations");
}
