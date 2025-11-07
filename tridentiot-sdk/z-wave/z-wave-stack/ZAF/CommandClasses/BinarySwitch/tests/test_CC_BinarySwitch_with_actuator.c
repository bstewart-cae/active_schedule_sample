#include <unity.h>
#include <CC_BinarySwitch.h>
#include <string.h>
#include <test_common.h>
#include "ZAF_CC_Invoker.h"
#include "ZW_TransportEndpoint_mock.h"
#include "ZAF_TSE_mock.h"
#include "CC_BinarySwitch_config_mock.h"
#include "SizeOf.h"
#include "CC_Supervision_mock.h"
#include "AppTimer_mock.h"
#include "SwTimer_mock.h"
#include "ZW_TransportEndpoint_mock.h"
#include "zaf_config_api_mock.h"

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
 * This test verifies the flow of Binary Switch Set V2 with True Status and Supervision.
 *
 * As the duration is set to zero the expected behavior is:
 * 1. Trigger True Status
 * 2. Don't transmit a Supervision Report, but return success so that CC Supervision will take
 *    care of transmitting the Supervision Report.
 */
void test_SWITCH_BINARY_SET_V2_with_TSE_and_Supervision(void)
{
  zaf_config_get_default_endpoint_IgnoreAndReturn(0);
  static cc_binary_switch_t binary_switches[] = {
    {
      .endpoint = 0,
      .default_duration = 60,
      .callback = switch_cb
    }
  };
  cc_binary_switch_get_config_ExpectAndReturn(binary_switches);
  cc_binary_switch_get_config_length_ExpectAndReturn(sizeof_array(binary_switches));

  AppTimerRegister_IgnoreAndReturn(true);

  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_BINARY);
  RECEIVE_OPTIONS_TYPE_EX rxOptions = {0};
  rxOptions.bSupervisionActive      = true; // Frame was received with Supervision.
  rxOptions.statusUpdate            = true; // Supervision Get has status updated enabled.
  const uint8_t SESSION_ID          = 10;
  rxOptions.sessionId               = SESSION_ID;

  const uint8_t VALUE = 50; // "On"
  const uint8_t DURATION = 0;

  uint8_t cmdFrame[] = {
    COMMAND_CLASS_SWITCH_BINARY,
    SWITCH_BINARY_SET,
    VALUE,
    DURATION
  };

  ZAF_TSE_Trigger_IgnoreAndReturn(true);

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
}

/*
 * This test verifies the flow of Binary Switch Set V2 with True Status and Supervision.
 *
 * As the duration is set to 0xFF the expected behavior is:
 * 1. Trigger True Status
 * 2. Don't transmit a Supervision Report, but return success so that CC Supervision will take
 *    care of transmitting the Supervision Report.
 */
void test_SWITCH_BINARY_SET_V2_with_TSE_and_Supervision_default_duration(void)
{
  zaf_config_get_default_endpoint_IgnoreAndReturn(0);
  static cc_binary_switch_t binary_switches[] = {
    {
      .endpoint = 0,
      .default_duration = 60,
      .callback = switch_cb
    }
  };
  cc_binary_switch_get_config_ExpectAndReturn(binary_switches);
  cc_binary_switch_get_config_length_ExpectAndReturn(sizeof_array(binary_switches));

  AppTimerRegister_IgnoreAndReturn(true);

  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_BINARY);
  RECEIVE_OPTIONS_TYPE_EX rxOptions = {0};
  rxOptions.bSupervisionActive      = true; // Frame was received with Supervision.
  rxOptions.statusUpdate            = true; // Supervision Get has status updated enabled.
  const uint8_t SESSION_ID          = 10;
  rxOptions.sessionId               = SESSION_ID;

  const uint8_t VALUE = 50; // "On"
  const uint8_t DURATION = 0xFF;

  uint8_t cmdFrame[] = {
    COMMAND_CLASS_SWITCH_BINARY,
    SWITCH_BINARY_SET,
    VALUE,
    DURATION
  };

  ZAF_TSE_Trigger_IgnoreAndReturn(true);

  cc_handler_input_t input = {
      .rx_options = &rxOptions,
      .frame = (ZW_APPLICATION_TX_BUFFER *) cmdFrame,
      .length = sizeof(cmdFrame)
  };

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0xFF
  };
  TimerIsActive_IgnoreAndReturn(true);
  TimerRestart_IgnoreAndReturn(ESWTIMER_STATUS_SUCCESS);
  is_multicast_IgnoreAndReturn(false);

  received_frame_status_t res = invoke_cc_handler(&input, &output);
  TEST_ASSERT_EQUAL(binary_switches[0].rxOpt.statusUpdate, false);

  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_WORKING, res);
  TEST_ASSERT_EQUAL_UINT8(0, output.length);
}
