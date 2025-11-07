/**
 * @file test_CC_DoorLock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <CC_DoorLock.h>
#include <mock_control.h>
#include <test_common.h>
#include <string.h>
#include <ZAF_TSE.h>
#include <cc_door_lock_config_api_mock.h>
#include <cc_door_lock_io_mock.h>
#include "zaf_transport_tx_mock.h"
#include "CC_Supervision_mock.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

static void cc_door_lock_configuration_set_frame_create(
    uint8_t * pFrame,
    uint8_t * pFrameLength,
    cc_door_lock_operation_type_t operation_type,
    uint8_t door_handles_mode,
    uint8_t lock_timeout_minutes,
    uint8_t lock_timeout_seconds);

static command_handler_input_t * door_lock_operation_set_frame_create(
    uint8_t mode,
    bool supervision_active,
    bool status_update,
    uint8_t session_id);

static command_handler_input_t * door_lock_operation_get_frame_create(void);
static command_handler_input_t * door_lock_configuration_get_frame_create(void);

void setUp(void) {
  cc_door_lock_migrate_Ignore();
  cc_door_lock_get_operation_type_IgnoreAndReturn(DOOR_OPERATION_CONST);
  cc_door_lock_get_supported_inside_handles_ExpectAndReturn(DOOR_HANDLE_DISABLED);
  cc_door_lock_get_supported_outside_handles_ExpectAndReturn(DOOR_HANDLE_1);
  cc_door_lock_read_IgnoreAndReturn(false);
  cc_door_lock_write_IgnoreAndReturn(true);

  ZAF_CC_reset_specific(COMMAND_CLASS_DOOR_LOCK);
}

void tearDown(void) {
}

void test_cc_door_lock_operation_type_t(void)
{
  TEST_ASSERT(0x01 == DOOR_OPERATION_CONST);
  TEST_ASSERT(0x02 == DOOR_OPERATION_TIMED);
}

/*
 * Tests that:
 * #1: A correct frame pointer is given to the external function being mocked.
 * #2: The returned frame status is SUCCESS if the external function being mocked returns true.
 */
void test_DOOR_LOCK_CONFIGURATION_SET_V2_valid_with_TSE(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  zaf_tx_options_t txOptions;
  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  // 1. No change.
  cc_door_lock_configuration_set_frame_create(
      pFrame,
      &commandLength,
      DOOR_OPERATION_CONST,
      DOOR_HANDLE_1 << 4, // Outside handle 1 will be set. | Outside (4 bit) | Inside (4 bit) |
      0,
      0);

  cc_door_lock_get_supported_inside_handles_ExpectAndReturn(DOOR_HANDLE_DISABLED);
  cc_door_lock_get_supported_outside_handles_ExpectAndReturn(DOOR_HANDLE_1);
  cc_door_lock_get_options_flags_ExpectAndReturn(0);
  cc_door_lock_get_max_auto_relock_time_ExpectAndReturn(0);
  cc_door_lock_get_max_hold_and_release_time_ExpectAndReturn(0);
  cc_door_lock_write_IgnoreAndReturn(true);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  cc_handler_input_t input = {
      .rx_options = &rxOptions,
      .frame = &frame,
      .length = commandLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, status, "Frame status doesn't match :("); // #2

  cc_door_lock_configuration_t configuration;
  ZW_DOOR_LOCK_CONFIGURATION_REPORT_V4_FRAME frame_2;

  configuration.type = DOOR_OPERATION_CONST;
  configuration.outsideDoorHandleMode = 0;
  configuration.insideDoorHandleMode = DOOR_HANDLE_1;
  configuration.lockTimeoutMin = DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED;
  configuration.lockTimeoutSec = DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED;
  configuration.autoRelockTime1 = 0;
  configuration.autoRelockTime2 = 0;
  configuration.holdAndReleaseTime1 = 0;
  configuration.holdAndReleaseTime2 = 0;
  configuration.reservedOptionsFlags = 0;
  frame_2.cmd = DOOR_LOCK_CONFIGURATION_REPORT_V4;
  frame_2.cmdClass = COMMAND_CLASS_DOOR_LOCK_V4;
  frame_2.operationType = configuration.type;
  frame_2.properties1 = ((configuration.outsideDoorHandleMode << 4) | configuration.insideDoorHandleMode);
  frame_2.lockTimeoutMinutes = configuration.lockTimeoutMin;
  frame_2.lockTimeoutSeconds = configuration.lockTimeoutSec;
  frame_2.autoRelockTime1 = configuration.autoRelockTime1;
  frame_2.autoRelockTime2 = configuration.autoRelockTime2;
  frame_2.holdAndReleaseTime1 = configuration.holdAndReleaseTime1;
  frame_2.holdAndReleaseTime2 = configuration.holdAndReleaseTime2;
  frame_2.properties2 = configuration.reservedOptionsFlags;

  memset((uint8_t*)&txOptions, 0x00, sizeof(zaf_tx_options_t));
  zaf_transport_tx_ExpectAndReturn((uint8_t *) &frame_2, sizeof(frame_2), ZAF_TSE_TXCallback, &txOptions, true);

  // 2. Change

  cc_door_lock_configuration_set_frame_create(
      pFrame,
      &commandLength,
      DOOR_OPERATION_CONST,
      DOOR_HANDLE_DISABLED,
      0,
      0);

  cc_door_lock_get_supported_inside_handles_ExpectAndReturn(DOOR_HANDLE_DISABLED);
  cc_door_lock_get_supported_outside_handles_ExpectAndReturn(0);
  cc_door_lock_get_options_flags_ExpectAndReturn(0);
  cc_door_lock_get_max_auto_relock_time_ExpectAndReturn(0);
  cc_door_lock_get_max_hold_and_release_time_ExpectAndReturn(0);
  cc_door_lock_write_IgnoreAndReturn(true);

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;  // callback function;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, status, "Frame status doesn't match :("); // #2

  // Fetch the callback passed to ZAF_TSE_Trigger().
  zaf_tse_callback_t cb = (zaf_tse_callback_t)pMock->actual_arg[0].p;

  configuration.insideDoorHandleMode = DOOR_HANDLE_DISABLED;
  frame_2.properties1 = ((configuration.outsideDoorHandleMode << 4) | configuration.insideDoorHandleMode);

  memset((uint8_t*)&txOptions, 0x00, sizeof(zaf_tx_options_t));
  zaf_transport_tx_ExpectAndReturn((uint8_t *) &frame_2, sizeof(frame_2), ZAF_TSE_TXCallback, &txOptions, true);
  cb(&txOptions, NULL);

  mock_calls_verify();
}

/*
 * Tests that:
 * #1:  Return RECEIVED_FRAME_STATUS_FAIL because SET function contains
 *      DOOR_OPERATION_TIMED, which isn't supported by the application.
 *      
 */
void test_DOOR_LOCK_CONFIGURATION_SET_V2_invalid(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  cc_door_lock_configuration_set_frame_create(
      pFrame,
      &commandLength,
      DOOR_OPERATION_TIMED,
      DOOR_HANDLE_1,
      0,
      0);

  cc_door_lock_get_supported_inside_handles_ExpectAndReturn(0);
  cc_door_lock_get_supported_outside_handles_ExpectAndReturn(DOOR_HANDLE_1);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  cc_handler_input_t input = {
      .rx_options = &rxOptions,
      .frame = &frame,
      .length = commandLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x02, status, "Frame status doesn't match :(");
  mock_calls_verify();
}

void test_DOOR_LOCK_OPERATION_SET_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  command_handler_input_t * p_chi = door_lock_operation_set_frame_create(DOOR_MODE_SECURED, false, false, 0);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;  // callback function;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_DOOR_LOCK_OPERATION_SET_supervision_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  uint8_t session_id = 1; // random supervision session id.
  bool status_update = true;
  command_handler_input_t * p_chi = door_lock_operation_set_frame_create(DOOR_MODE_SECURED, true, status_update, session_id);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  mock_call_expect(TO_STR(is_multicast),&pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;  // callback function;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = true;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 2
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_WORKING, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_DOOR_LOCK_OPERATION_SET_failure(void)
{
  mock_calls_clear();

  command_handler_input_t * p_chi = door_lock_operation_set_frame_create(DOOR_MODE_UNKNOWN, false, false, 0);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_DOOR_LOCK_OPERATION_GET_V4_successful(void)
{
  mock_calls_clear();

  command_handler_input_t * p_chi = door_lock_operation_get_frame_create();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_DOOR_LOCK_V2,
                                     DOOR_LOCK_OPERATION_REPORT_V2,
                                     DOOR_MODE_UNSECURE,
                                     DOOR_HANDLE_1 << 4, // Outside handle 1 will be set. | Outside (4 bit) | Inside (4 bit) |
                                     0x06,
                                     DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED,
                                     DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED,
                                     DOOR_MODE_UNSECURE,
                                     0x02 // This 2 seconds is comming from the simulated door lock hw.
  };

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_REPORT), output.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_REPORT, (uint8_t *)&frame_out, output.length);

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_DOOR_LOCK_CONFIGURATION_GET_V4_successful(void)
{
  mock_calls_clear();

  command_handler_input_t * p_chi = door_lock_configuration_get_frame_create();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_DOOR_LOCK_V2,
                                     DOOR_LOCK_CONFIGURATION_REPORT_V2,
                                     DOOR_OPERATION_CONST,
                                     DOOR_HANDLE_1 << 4, // Outside handle 1 will be set. | Outside (4 bit) | Inside (4 bit) |
                                     DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED,
                                     DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED,
                                     0x00,
                                     0x00,
                                     0x00,
                                     0x00,
                                     0x00
  };

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_REPORT), output.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_REPORT, (uint8_t *)&frame_out, output.length);

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**
 * Creates a DOOR_LOCK_CONFIGURATION_SET frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 * @param operation_type Operation type.
 * @param door_handles_mode Contains both indoor and outdoor handle modes.
 * @param lock_timeout_minutes Lock timeout minutes.
 * @param lock_timeout_seconds Lock timeout seconds.
 */
static void cc_door_lock_configuration_set_frame_create(
    uint8_t * pFrame,
    uint8_t * pFrameLength,
    cc_door_lock_operation_type_t operation_type,
    uint8_t door_handles_mode,
    uint8_t lock_timeout_minutes,
    uint8_t lock_timeout_seconds)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_DOOR_LOCK;
  pFrame[frameCount++] = DOOR_LOCK_CONFIGURATION_SET_V2;
  pFrame[frameCount++] = operation_type;
  pFrame[frameCount++] = door_handles_mode;
  pFrame[frameCount++] = lock_timeout_minutes;
  pFrame[frameCount++] = lock_timeout_seconds;
  *pFrameLength = frameCount;
}

static command_handler_input_t * door_lock_operation_set_frame_create(
    uint8_t mode,
    bool supervision_active,
    bool status_update,
    uint8_t session_id)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_DOOR_LOCK;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = DOOR_LOCK_OPERATION_SET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = mode;

  if (supervision_active) {
    p_chi->rxOptions.bSupervisionActive = 1;
    p_chi->rxOptions.sessionId = session_id;
    p_chi->rxOptions.statusUpdate = status_update ? 1 : 0;
  }
  return p_chi;
}

static command_handler_input_t * door_lock_operation_get_frame_create(void)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_DOOR_LOCK;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = DOOR_LOCK_OPERATION_GET;
  return p_chi;
}

static command_handler_input_t * door_lock_configuration_get_frame_create(void)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_DOOR_LOCK;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = DOOR_LOCK_CONFIGURATION_GET;
  return p_chi;
}

static command_handler_input_t * door_lock_capabilities_get_frame_create(void)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_DOOR_LOCK;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = DOOR_LOCK_CAPABILITIES_GET_V4;
  return p_chi;
}


void test_DOOR_LOCK_CAPABILITIES_GET_V4_successful(void)
{
  mock_calls_clear();

  command_handler_input_t * p_chi = door_lock_capabilities_get_frame_create();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_door_lock_get_supported_inside_handles_ExpectAndReturn(0xB);
  cc_door_lock_get_supported_outside_handles_ExpectAndReturn(0xA);

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_DOOR_LOCK_V4,
                                     DOOR_LOCK_CAPABILITIES_REPORT_V4,
                                     0x01,
                                     0x02,
                                     0x02,
                                     0x00,
                                     0xFF,
                                     0xAB,  /* Supported Outside / Inside Handle mode bitmasks*/
                                     0x06,  /* Supported Door Components */
                                     0x00
  };

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  cc_handler_output_t output = {
      .frame = &frame_out,
      .length = 0
  };
  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_REPORT), output.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_REPORT, (uint8_t *)&frame_out, output.length);

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}
