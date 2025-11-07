/**
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2024 Silicon Laboratories Inc.
 */
#include <string.h>
#include <unity.h>
#include "test_common.h"
#include "ZW_classcmd.h"
#include "ZAF_CC_Invoker.h"
#include "ZAF_types.h"
#include "association_plus_base_mock.h"
#include "zaf_transport_tx_mock.h"
#include "ZAF_TSE_mock.h"
#include "cc_user_credential_config_api_mock.h"
#include "cc_user_credential_io_mock.h"

void setUpSuite(void)
{
  ZAF_TSE_Trigger_IgnoreAndReturn(true);
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

/**
 * @brief Mock handlers and data for AC code fetch operation.
 */

/**
 * @brief Valid AC
 */
static uint8_t ac_good[U3C_CREDENTIAL_TYPE_PIN_CODE_MAX_LENGTH_REQUIREMENT] =
{ 0x33, 0x34, 0x39, 0x34 };

/**
 * @brief Tests that the outgoing report tx works properly and all fields
 * read acceptable values when responding to a valid Get frame, to fulfill
 * the following requirements:
 *
 * CC:0083.01.1B.11.001
 */
void test_USER_CREDENTIAL_admin_pin_code_report(void)
{
#define AC_LENGTH 4
#define AC_FRAME_SIZE (sizeof(ZW_ADMIN_PIN_CODE_REPORT_1BYTE_FRAME) - 1 + AC_LENGTH)
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_GET_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_GET
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeGetFrame
    = incomingFrame;
  // Set up code and admin code data

  // "3494" as a byte string
  uint8_t admin_code[] = {
    0x33, 0x34, 0x39, 0x34
  };

  // Create expected outgoing frame
  size_t index = 0;
  uint8_t expectedOutput[AC_FRAME_SIZE] = { 0 };
  expectedOutput[index++] = COMMAND_CLASS_USER_CREDENTIAL;
  expectedOutput[index++] = ADMIN_PIN_CODE_REPORT;
  expectedOutput[index++] = ((uint8_t)((ADMIN_CODE_OPERATION_RESULT_GET_RESP << 4) & 0xF0) | (uint8_t)(AC_LENGTH & 0x0F));
  memcpy(&expectedOutput[index], admin_code, AC_LENGTH);

  // Create 'successful' credential get frame
  u3c_admin_code_metadata_t existing_ac = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_GET_RESP,
  };
  memcpy(existing_ac.code_data, ac_good, existing_ac.code_length);
  // Empty tx_opt_t
  zaf_tx_options_t tx_opt = { 0 };

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  CC_UserCredential_get_admin_code_info_ExpectAndReturn(NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_admin_code_info_ReturnThruPtr_code(&existing_ac);
  CC_UserCredential_get_admin_code_info_IgnoreArg_code();
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnMemThruPtr_tx_options(&tx_opt, sizeof(zaf_tx_options_t));
  zaf_transport_tx_ExpectWithArrayAndReturn(
    expectedOutput,
    AC_FRAME_SIZE,
    AC_FRAME_SIZE,
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command, expect nothing in the output
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Admin PIN Code Get was not answered."
    );

  zaf_transport_tx_StopIgnore();
#undef AC_LENGTH
#undef AC_FRAME_SIZE
}

/**
 * @brief Tests a get operation when AC is not supported according to the following
 * requirements:
 * CC:0083.01.1B.13.001
 */
void test_USER_CREDENTIAL_admin_pin_code_get_not_supported(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_GET_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_GET
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeGetFrame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(false);
  // This should immediately fail
  // Process command
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_NO_SUPPORT, status,
    "Admin PIN code should not be retrieved with no support"
    );
}

/**
 * @brief Tests valid set operation.
 */
void test_USER_CREDENTIAL_admin_pin_code_set_valid(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x04,
    0x34,
    0x39,
    0x34,
    0x33,
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;
  input.rxOptions.sourceNode.nodeId = 1;
  input.rxOptions.destNode.nodeId = 2;

  // Create Expected output from set
  u3c_admin_code_metadata_t set_output = {
    .result = ADMIN_CODE_OPERATION_RESULT_MODIFIED,
    .code_length = 4,
    .code_data = { 0x34, 0x39, 0x34, 0x33 }
  };

  // Create Expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_AdminPinCodeReport4byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = ADMIN_PIN_CODE_REPORT,
      .properties1 = (uint8_t)((ADMIN_CODE_OPERATION_RESULT_MODIFIED << 4) | 0x04),
      .adminCode1 = 0x34,
      .adminCode2 = 0x39,
      .adminCode3 = 0x34,
      .adminCode4 = 0x33,
    }
  };

  // Mock Existing Credential
  u3c_credential_metadata_t existing = {
    .length = 4,
    .slot = 1,
    .uuid = 1,
    .type = CREDENTIAL_TYPE_PIN_CODE,
  };

  uint8_t existing_data[] = {
    0x31, 0x31, 0x31, 0x31
  };

  // Empty tx_opt_t for calls later
  zaf_tx_options_t tx_opt = { 0 };

  // Create 'successful' credential get frame
  u3c_admin_code_metadata_t existing_ac = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_GET_RESP,
  };
  memcpy(existing_ac.code_data, ac_good, existing_ac.code_length);

  u3c_credential_type nextType = CREDENTIAL_TYPE_PIN_CODE;
  uint16_t nextSlot = 1;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);

  // Mock check for existing admin code
  CC_UserCredential_get_admin_code_info_ExpectAndReturn(NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_admin_code_info_ReturnThruPtr_code(&existing_ac);
  CC_UserCredential_get_admin_code_info_IgnoreArg_code();

  CC_UserCredential_get_credential_StopIgnore();
  // Let's put one user in the db with a different pin code to ensure that the comparison code works
  CC_UserCredential_get_next_user_ExpectAndReturn(0, 1);
  CC_UserCredential_get_next_credential_ExpectAnyArgsAndReturn(true);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&nextType);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&nextSlot);
  CC_UserCredential_get_credential_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&existing);
  CC_UserCredential_get_credential_ReturnThruPtr_credential_data((uint8_t*)existing_data);
  CC_UserCredential_get_next_credential_ExpectAnyArgsAndReturn(false);
  CC_UserCredential_get_next_user_ExpectAnyArgsAndReturn(0);

  // Should pass previous checks, now we set and transmit report
  CC_UserCredential_set_admin_code_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_set_admin_code_ReturnThruPtr_code(&set_output);
  MULTICHAN_NODE_ID nodes[1] = { 0 };
  nodes[0].node.nodeId = 1;
  uint8_t listLen = 1;
  destination_info_t * pNodeList = (destination_info_t *)&nodes;
  handleAssociationGetnodeList_ExpectAndReturn(1, 0, NULL, NULL, NODE_LIST_STATUS_SUCCESS);
  handleAssociationGetnodeList_ReturnArrayThruPtr_ppListOfNodes(&pNodeList, listLen);
  handleAssociationGetnodeList_ReturnThruPtr_pListLen(&listLen);
  handleAssociationGetnodeList_IgnoreArg_ppListOfNodes();
  handleAssociationGetnodeList_IgnoreArg_pListLen();
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnThruPtr_tx_options(&tx_opt);
  zaf_transport_tx_ExpectWithArrayAndReturn(
    (uint8_t *)&expectedOutput,
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "Valid admin pin code should set properly"
    );

  zaf_transport_tx_StopIgnore();
}

/**
 * @brief Tests that attempting to set the AC to its current value will fail.
 * CC:0083.01.1A.13.004
 */
void test_USER_CREDENTIAL_admin_pin_code_set_invalid_duplicate_AC(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x04,
    0x33,
    0x34,
    0x39,
    0x34
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;

  // Create Expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_AdminPinCodeReport4byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = ADMIN_PIN_CODE_REPORT,
      .properties1 = (uint8_t)((ADMIN_CODE_OPERATION_RESULT_UNMODIFIED << 4) | 0x04),
      .adminCode1 = 0x33,
      .adminCode2 = 0x34,
      .adminCode3 = 0x39,
      .adminCode4 = 0x34,
    }
  };

  // Empty tx_opt_t for calls later
  zaf_tx_options_t tx_opt = { 0 };

  // Create 'successful' credential get frame
  u3c_admin_code_metadata_t existing_ac = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_GET_RESP,
  };
  memcpy(existing_ac.code_data, ac_good, existing_ac.code_length);

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  CC_UserCredential_get_admin_code_info_ExpectAndReturn(NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_admin_code_info_ReturnThruPtr_code(&existing_ac);
  CC_UserCredential_get_admin_code_info_IgnoreArg_code();
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnThruPtr_tx_options(&tx_opt);
  zaf_transport_tx_ExpectWithArrayAndReturn(
    (uint8_t *)&expectedOutput,
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "Device should recognize duplicate admin code"
    );

  zaf_transport_tx_StopIgnore();
}

/**
 * @brief Tests set operation when AC is not supported. Packet can be ignored per
 * requirement CC:0083.01.1A.13.002
 */
void test_USER_CREDENTIAL_admin_pin_code_set_not_supported(void)
{
  #define AC_LENGTH 4
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_1BYTE_FRAME incomingFrame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = ADMIN_PIN_CODE_SET,
    .properties1 = (uint8_t)(0x0F & AC_LENGTH)
  };

  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet1byteFrame
    = incomingFrame;
  input.frameLength = sizeof(ZW_ADMIN_PIN_CODE_SET_1BYTE_FRAME) - 1 + AC_LENGTH;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(false);
  // Should fail almost immediately.
  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_NO_SUPPORT, status,
    "AC Set should be ignored as it is not supported"
    );
  #undef AC_LENGTH
}

/**
 * @brief Tests that output of admin pin code set operation with an unsupported
 * ASCII character fulfills the following requirements:
 * CC:0083.01.1A.11.012
 */
void test_USER_CREDENTIAL_admin_pin_code_set_bad_encoding(void)
{
  #define AC_LENGTH 4
  // Create incoming frame with improperly encoded digits, but otherwise valid
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x04,
    0x80,
    0x81,
    0x82,
    0x83,
  };

  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame2 = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x04,
    0x33,
    0x34,
    0x89,
    0x34,
  };

  // Test one set of improperly encoded digits
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;
  input.frameLength = sizeof(ZW_ADMIN_PIN_CODE_SET_1BYTE_FRAME) - 1 + AC_LENGTH;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  // Should fail almost immediately.
  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  // Test other set of improperly encoded digits
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame2;
  input.frameLength = sizeof(ZW_ADMIN_PIN_CODE_SET_1BYTE_FRAME) - 1 + AC_LENGTH;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  // Should fail almost immediately.
  // Invoke handler
  received_frame_status_t status2 =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "Admin Pin code low encoding missed"
    );

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status2,
    "Admin Pin coding high encoding missed"
    );
  #undef AC_LENGTH
}

/**
 * @brief Tests valid ACD disable with incoming code length of 0 to fulfill
 * the following requirements:
 * CC:0083.01.1A.11.006
 */
void test_USER_CREDENTIAL_admin_pin_code_set_acd_disable_valid(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x00,
    0x33, // should throw away this data
    0x34,
    0x39,
    0x34,
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;
  input.rxOptions.sourceNode.nodeId = 1;
  input.rxOptions.destNode.nodeId = 2;

  // Create Expected output from set
  u3c_admin_code_metadata_t set_output = {
    .result = ADMIN_CODE_OPERATION_RESULT_MODIFIED,
    .code_length = 0,
  };

  // Create Expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_AdminPinCodeReport1byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = ADMIN_PIN_CODE_REPORT,
      .properties1 = (uint8_t)((ADMIN_CODE_OPERATION_RESULT_MODIFIED << 4) | 0x00),
      .adminCode1 = 0
    }
  };

  // Empty tx_opt_t for calls later
  zaf_tx_options_t tx_opt = { 0 };

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  cc_user_credential_get_admin_code_deactivate_supported_ExpectAndReturn(true);
  CC_UserCredential_set_admin_code_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_set_admin_code_ReturnThruPtr_code(&set_output);
  MULTICHAN_NODE_ID nodes[1] = { 0 };
  nodes[0].node.nodeId = 1;
  uint8_t listLen = 1;
  destination_info_t * pNodeList = (destination_info_t *)&nodes;
  handleAssociationGetnodeList_ExpectAndReturn(1, 0, NULL, NULL, NODE_LIST_STATUS_SUCCESS);
  handleAssociationGetnodeList_ReturnArrayThruPtr_ppListOfNodes(&pNodeList, listLen);
  handleAssociationGetnodeList_ReturnThruPtr_pListLen(&listLen);
  handleAssociationGetnodeList_IgnoreArg_ppListOfNodes();
  handleAssociationGetnodeList_IgnoreArg_pListLen();
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnThruPtr_tx_options(&tx_opt);
  zaf_transport_tx_ExpectWithArrayAndReturn(
    (uint8_t *)&expectedOutput,
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_1BYTE_FRAME) - 1 + 0, // not strictly necessary, just for readability
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_1BYTE_FRAME) - 1 + 0,
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "AC should allow disabling"
    );

  zaf_transport_tx_StopIgnore();
}
/**
 * @brief Tests ACD disable with no ACD support to fulfill
 * the following requirements:
 * CC:0083.01.1A.13.003
 */
void test_USER_CREDENTIAL_admin_pin_code_set_acd_disable_invalid(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x00,
    0x33, // should throw away this data
    0x34,
    0x39,
    0x34,
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;

  // Create Expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_AdminPinCodeReport1byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = ADMIN_PIN_CODE_REPORT,
      .properties1 = (uint8_t)((ADMIN_CODE_OPERATION_RESULT_ERROR_ACD_NOT_SUPPORTED << 4) | 0x00),
      .adminCode1 = 0
    }
  };

  // Empty tx_opt_t for calls later
  zaf_tx_options_t tx_opt = { 0 };

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  cc_user_credential_get_admin_code_deactivate_supported_ExpectAndReturn(false);
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnThruPtr_tx_options(&tx_opt);
  zaf_transport_tx_ExpectWithArrayAndReturn(
    (uint8_t *)&expectedOutput,
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_1BYTE_FRAME) - 1 + 0, // not strictly necessary, just for readability
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_1BYTE_FRAME) - 1 + 0,
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_NO_SUPPORT, status,
    "Test should have failed with no support for ACD"
    );
  zaf_transport_tx_StopIgnore();
}

/**
 * @brief Test set operation where the provided code is an invalid length
 * to fulfill the following requirements:
 * CC:0083.01.1A.11.004
 *
 */
void test_USER_CREDENTIAL_admin_pin_code_set_invalid_length(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  // Length of 15 characters, out of spec
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame1 = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x0F,
    0x33, // should throw away this data
    0x34,
    0x39,
    0x34,
  };
  // Length of 2 characters, out of spec
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame2 = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x02,
    0x33, // should throw away this data
    0x34,
    0x39,
    0x34,
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame1;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  // should fail immediately

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame2;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  // should fail immediately

  // Invoke handler
  received_frame_status_t status2 =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "Test should have failed as input length is too long"
    );

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status2,
    "Test should have failed as input length is too short"
    );
}

/**
 * @brief Tests that an Admin Code that matches a PIN Code in the credential database fails in
 * order to fulfill the following requirements:
 * CC:0083.01.1A.13.005
 */
void test_USER_CREDENTIAL_admin_pin_code_set_test_duplicate_credential(void)
{
  // Create (otherwise valid) incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x04,
    0x33,
    0x34,
    0x39,
    0x34,
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;

  // Create Expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_AdminPinCodeReport4byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = ADMIN_PIN_CODE_REPORT,
      .properties1 = (uint8_t)((ADMIN_CODE_OPERATION_RESULT_FAIL_DUPLICATE_CRED << 4) | 0x04),
      .adminCode1 = 0x33,
      .adminCode2 = 0x34,
      .adminCode3 = 0x39,
      .adminCode4 = 0x34,
    }
  };

  // Mock Existing Credential
  u3c_credential_metadata_t existing = {
    .length = 4,
    .slot = 1,
    .uuid = 1,
    .type = CREDENTIAL_TYPE_PIN_CODE,
  };

  // Empty tx_opt_t for calls later
  zaf_tx_options_t tx_opt = { 0 };

  uint8_t existing_code_data[] = {
    0x32, 0x32, 0x32, 0x32
  };

  // Create 'successful' credential get frame
  u3c_admin_code_metadata_t existing_ac = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_GET_RESP,
  };
  memcpy(existing_ac.code_data, existing_code_data, existing_ac.code_length);

  u3c_credential_type nextType = CREDENTIAL_TYPE_PIN_CODE;
  uint16_t nextSlot = 1;

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);

  // Mock check for existing admin code
  CC_UserCredential_get_admin_code_info_ExpectAndReturn(NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_admin_code_info_ReturnThruPtr_code(&existing_ac);
  CC_UserCredential_get_admin_code_info_IgnoreArg_code();

  // Let's put one user in the db with the same pin code
  CC_UserCredential_get_next_user_ExpectAndReturn(0, 1);
  CC_UserCredential_get_next_credential_ExpectAnyArgsAndReturn(true);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&nextType);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&nextSlot);
  CC_UserCredential_get_credential_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&existing);
  CC_UserCredential_get_credential_ReturnMemThruPtr_credential_data((uint8_t*)ac_good, 4); //3494 mock pin code

  // Should fail that check, move to reporting bad set
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnThruPtr_tx_options(&tx_opt);
  zaf_transport_tx_ExpectWithArrayAndReturn(
    (uint8_t *)&expectedOutput,
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "AC Set should fail with a duplicate credential in the database"
    );

  zaf_transport_tx_StopIgnore();
}

/**
 * @brief Tests database error code path
 */
void test_USER_CREDENTIAL_admin_pin_code_set_test_database_error(void)
{
  // Create (otherwise valid) incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_ADMIN_PIN_CODE_SET_4BYTE_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    ADMIN_PIN_CODE_SET,
    0x04,
    0x33,
    0x34,
    0x39,
    0x34,
  };
  input.frame.as_zw_application_tx_buffer.ZW_AdminPinCodeSet4byteFrame
    = incomingFrame;

  // Create Expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_AdminPinCodeReport4byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = ADMIN_PIN_CODE_REPORT,
      .properties1 = (uint8_t)((ADMIN_CODE_OPERATION_RESULT_ERROR_NODE << 4) | 0x04),
      .adminCode1 = 0x31,
      .adminCode2 = 0x32,
      .adminCode3 = 0x33,
      .adminCode4 = 0x34,
    }
  };

  // Empty tx_opt_t for calls later
  zaf_tx_options_t tx_opt = { 0 };

  // Create 'successful' credential get frame
  u3c_admin_code_metadata_t existing_ac = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_GET_RESP,
  };
  memcpy(existing_ac.code_data, ac_good, existing_ac.code_length);

  // Set up mock calls
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);

  // Mock check for existing admin code
  CC_UserCredential_get_admin_code_info_ExpectAndReturn(NULL, U3C_DB_OPERATION_RESULT_ERROR);
  CC_UserCredential_get_admin_code_info_ReturnThruPtr_code(&existing_ac);
  CC_UserCredential_get_admin_code_info_IgnoreArg_code();
  zaf_transport_rx_to_tx_options_ExpectAnyArgs();
  zaf_transport_rx_to_tx_options_ReturnThruPtr_tx_options(&tx_opt);
  zaf_transport_tx_ExpectWithArrayAndReturn(
    (uint8_t *)&expectedOutput,
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    sizeof(ZW_ADMIN_PIN_CODE_REPORT_4BYTE_FRAME),
    NULL,
    &tx_opt,
    sizeof(zaf_tx_options_t),
    true
    );
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Invoke handler
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "AC IO operation should fail but should still successfully receive and transmit reports"
    );

  zaf_transport_tx_StopIgnore();
}
