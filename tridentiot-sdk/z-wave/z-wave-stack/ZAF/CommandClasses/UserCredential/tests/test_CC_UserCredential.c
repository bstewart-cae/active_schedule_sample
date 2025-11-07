/**
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2024 Silicon Laboratories Inc.
 */
#include <unity.h>
#include "test_common.h"
#include "ZW_classcmd.h"
#include "ZAF_CC_Invoker.h"
#include "ZAF_types.h"
#include <stdbool.h>
#include <string.h>
#include "cc_user_credential_config_api_mock.h"
#include "cc_user_credential_io_mock.h"
#include "zaf_transport_tx_mock.h"
#include "association_plus_base_mock.h"
#include "ZAF_TSE_mock.h"

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
 * Verifies that an existing User's data is reported correctly.
 */
void test_USER_CREDENTIAL_user_get(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_USER_GET_FRAME incomingFrame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_GET,
    .userUniqueIdentifier1 = 0x00,
    .userUniqueIdentifier2 = 0x06
  };
  input.frameLength = sizeof(ZW_USER_GET_FRAME);
  input.frame.as_zw_application_tx_buffer.ZW_UserGetFrame = incomingFrame;
  const uint8_t UUID = 6;

  // Set up mock calls
  char name[] = "Test-User-6";
  u3c_user_t user = {
    .unique_identifier = UUID,
    .modifier_type = MODIFIER_TYPE_LOCALLY,
    .modifier_node_id = 2,
    .type = USER_TYPE_GENERAL,
    .active = true,
    .credential_rule = CREDENTIAL_RULE_SINGLE,
    .expiring_timeout_minutes = 0,
    .name_encoding = USER_NAME_ENCODING_STANDARD_ASCII,
    .name_length = sizeof(name) - 1
  };

  CC_UserCredential_get_next_user_ExpectAndReturn(UUID, 8); // Dont care the next user ID and expect it to be returned as 8.
  CC_UserCredential_get_user_ExpectAndReturn(UUID, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_ReturnThruPtr_user(&user);
  CC_UserCredential_get_user_ReturnArrayThruPtr_name((uint8_t *)name, user.name_length);
  CC_UserCredential_get_user_IgnoreArg_user();
  CC_UserCredential_get_user_IgnoreArg_name();
  zaf_transport_rx_to_tx_options_Ignore();

  // Create expected outgoing frame
  ZW_USER_REPORT_1BYTE_FRAME expectedOutput = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_REPORT,
    .nextUserUniqueIdentifier1 = 0x00,
    .nextUserUniqueIdentifier2 = 0x08,
    .userModifierType = MODIFIER_TYPE_LOCALLY,
    .userModifierNodeId1 = 0x00,
    .userModifierNodeId2 = 0x02,
    .userUniqueIdentifier1 = 0x00,
    .userUniqueIdentifier2 = 0x06,
    .userType = USER_TYPE_GENERAL,
    .properties1 = 0x01,
    .credentialRule = CREDENTIAL_RULE_SINGLE,
    .expiringTimeoutMinutes1 = 0x00,
    .expiringTimeoutMinutes2 = 0x00,
    .properties2 = 0x00,
    .userNameLength = user.name_length
  };
  memcpy(&expectedOutput.userName1, name, user.name_length);
  uint8_t expectedSize = sizeof(ZW_USER_REPORT_1BYTE_FRAME) - 1 + user.name_length;

  zaf_transport_tx_ExpectWithArrayAndReturn((uint8_t *)&expectedOutput, 1, expectedSize, NULL, NULL, 1, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status =
    invoke_cc_handler_v2(
      &input.rxOptions, &input.frame.as_zw_application_tx_buffer,
      input.frameLength, NULL, NULL);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The User Get was not answered."
    );
}

// CC:0083.01.05.11.004
// CC:0083.01.05.13.005
// Test that sends a USER_SET command with a valid user unique identifier, and verifies that its accepted, and a report is sent
void test_USER_CREDENTIAL_user_set_valid_user_unique_identifier(void)
{
  uint16_t user_unique_identifier = 0x1A; // Some random but valid user unique identifier

  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  input.rxOptions.sourceNode.nodeId = 1;
  input.rxOptions.destNode.nodeId = 2;
  ZW_USER_SET_1BYTE_FRAME incomingFrame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_SET,
    .properties1 = USER_SET_OPERATION_TYPE_ADD, // Operation: Add
    .userUniqueIdentifier1 = (user_unique_identifier >> 8) & 0xFF,
    .userUniqueIdentifier2 = user_unique_identifier & 0xFF,   // User Unique Identifier
    .userType = USER_TYPE_GENERAL,
    .properties2 = 0x00,   // User is not Active
    .credentialRule = CREDENTIAL_RULE_SINGLE,
    .expiringTimeoutMinutes1 = 0x00,
    .expiringTimeoutMinutes2 = 0x00,   // Expiring timout = 0
    .properties3 = USER_NAME_ENCODING_STANDARD_ASCII,
    .userNameLength = 0,   // User Name length = 0, generate Default Name
    .userName1 = 0x00   // First byte of User Name (ignored)
  };
  input.frame.as_zw_application_tx_buffer.ZW_UserSet1byteFrame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(user_unique_identifier + 1); // Don't care, just needs to be bigger than the user_unique_identifier
  cc_user_credential_get_max_length_of_user_name_ExpectAndReturn(40); // Ignore the max name length
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_GENERAL, true);
  cc_user_credential_is_credential_rule_supported_ExpectAndReturn(CREDENTIAL_RULE_SINGLE, true);

  // Mock the call to the user credential add user
  CC_UserCredential_add_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_next_user_ExpectAndReturn(user_unique_identifier, 0);

  uint8_t listLen = 1;
  MULTICHAN_NODE_ID nodes[1] = { 0 };
  nodes[0].node.nodeId = 1;
  destination_info_t * pNodeList = (destination_info_t *)&nodes;
  handleAssociationGetnodeList_ExpectAndReturn(1, 0, NULL, NULL, NODE_LIST_STATUS_SUCCESS);
  handleAssociationGetnodeList_ReturnArrayThruPtr_ppListOfNodes(&pNodeList, listLen);
  handleAssociationGetnodeList_ReturnThruPtr_pListLen(&listLen);
  handleAssociationGetnodeList_IgnoreArg_ppListOfNodes();
  handleAssociationGetnodeList_IgnoreArg_pListLen();
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_IgnoreAndReturn(true);

  received_frame_status_t status = invoke_cc_handler_v2(
    &input.rxOptions,
    &input.frame.as_zw_application_tx_buffer,
    input.frameLength, NULL, 0
    );

  // Verify that the command was accepted
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The USER_SET command was not accepted."
    );
}

//CC:0083.01.05.11.004
//CC:0083.01.05.11.006
//Test that sends a USER_SET command with an invalid "0" user unique identifier, and verifies that its ignored
void test_USER_CREDENTIAL_UserSet_InvalidUniqueIdentifier_Zero(void)
{
  // Create incoming frame with User Unique Identifier set to 0 (invalid)
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_USER_SET_1BYTE_FRAME incomingFrame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_SET,
    .properties1 = USER_SET_OPERATION_TYPE_ADD, // Operation: Add
    .userUniqueIdentifier1 = 0x00, // MSB of User Unique Identifier = 0
    .userUniqueIdentifier2 = 0x00, // LSB of User Unique Identifier = 0, making it invalid
    .userType = USER_TYPE_GENERAL,
    .properties2 = 0x01, // User is Active
    .expiringTimeoutMinutes1 = 0x00, // Expiring timeout = 0
    .expiringTimeoutMinutes2 = 0x00,
    .properties3 = USER_NAME_ENCODING_STANDARD_ASCII,
    .userNameLength = 0, // User Name length = 0, generate Default Name
    .userName1 = 0x00   // First byte of User Name (ignored in this context)
  };
  input.frame.as_zw_application_tx_buffer.ZW_UserSet1byteFrame = incomingFrame;

  // Set up mock calls to simulate environment and expected behavior
  // These expectations might need adjustment based on your system's design on handling invalid identifiers.
  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(0xFFFF); // Max valid User Unique Identifier
  // Process command
  received_frame_status_t status = invoke_cc_handler_v2(&input.rxOptions, &input.frame.as_zw_application_tx_buffer, input.frameLength, NULL, 0);

  // Verify that the handler correctly handles the invalid User Unique Identifier
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Handler incorrectly processed an invalid User Unique Identifier of 0.");
}

// CC:0083.01.05.11.014
void test_USER_SET_With_UserUniqueIdentifier_Greater_Than_Max_Should_Be_Ignored(void)
{
  // Set up the incoming USER_SET frame with a User Unique Identifier greater than the maximum allowed
  ZW_USER_SET_1BYTE_FRAME incomingFrame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_SET,
    .properties1 = USER_SET_OPERATION_TYPE_ADD,
    .userUniqueIdentifier1 = 0xFF, // Assuming max is lower, use a value that is guaranteed to be higher
    .userUniqueIdentifier2 = 0xFF,
    .userType = USER_TYPE_GENERAL,
    .properties2 = 0x01, // User is Active
    .credentialRule = CREDENTIAL_RULE_SINGLE,
    .expiringTimeoutMinutes1 = 0x00,
    .expiringTimeoutMinutes2 = 0x00,
    .properties3 = USER_NAME_ENCODING_STANDARD_ASCII,
    .userNameLength = 0, // No name provided
    .userName1 = 0x00 // First byte of User Name (ignored)
  };

  // Set up the Command Handler input
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  input.frame.as_zw_application_tx_buffer.ZW_UserSet1byteFrame = incomingFrame;

  // Expect the call to retrieve the max user unique identifiers and return a value lower than the one in the frame
  uint16_t maxSupportedIdentifiers = 0x00FF; // Example maximum
  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(maxSupportedIdentifiers);

  // Process the command
  received_frame_status_t status = invoke_cc_handler_v2(
    &input.rxOptions,
    &input.frame.as_zw_application_tx_buffer,
    input.frameLength, NULL, 0
    );

  // Verify that the command was ignored
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "Expected the USER_SET command with User Unique Identifier greater than the max to be ignored."
    );
}

// CC:0083.01.05.11.007
// CC:0083.01.05.11.008
// CC:0083.01.05.11.009
// CC:0083.01.05.11.010
// Test that sends 2 USER_SET (adding two users), and after send a USER_SET (UUID=0, USER_REMOVE), and verifies that all users are removed
void test_USER_CREDENTIAL_user_remove_all_users(void)
{
  #define NODE_ID 1
  // Set up AGI
  uint8_t listLen = 1;
  MULTICHAN_NODE_ID nodes[1] = { 0 };
  nodes[0].node.nodeId = NODE_ID;
  destination_info_t * pNodeList = (destination_info_t *)&nodes;

  // Create incoming frame of the first USER (UUID = 1)
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  input.rxOptions.sourceNode.nodeId = NODE_ID;
  ZW_USER_SET_1BYTE_FRAME incomingFrame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_SET,
    .properties1 = USER_SET_OPERATION_TYPE_ADD,
    .userUniqueIdentifier1 = 0x00,
    .userUniqueIdentifier2 = 0x01,   // User Unique Identifier = 1
    .userType = USER_TYPE_GENERAL,
    .properties2 = 0x00,   // User is not Active
    .credentialRule = CREDENTIAL_RULE_SINGLE,
    .expiringTimeoutMinutes1 = 0x00,
    .expiringTimeoutMinutes2 = 0x00,   // Expiring timout = 0
    .properties3 = USER_NAME_ENCODING_STANDARD_ASCII,
    .userNameLength = 0,   // User Name length = 0, generate Default Name
    .userName1 = 0x00   // First byte of User Name (ignored)
  };
  input.frame.as_zw_application_tx_buffer.ZW_UserSet1byteFrame
    = incomingFrame;
  input.rxOptions.sourceNode.nodeId = 1;
  input.rxOptions.destNode.nodeId = 2;

  // Set up mock calls
  cc_user_credential_get_max_user_unique_idenfitiers_IgnoreAndReturn(10); // Don't care, just needs to be bigger than the user_unique_identifier
  cc_user_credential_get_max_length_of_user_name_IgnoreAndReturn(40); // Ignore the max name length
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_GENERAL, true);
  cc_user_credential_is_credential_rule_supported_ExpectAndReturn(CREDENTIAL_RULE_SINGLE, true);

  // Mock the call to the user credential add user
  CC_UserCredential_add_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_next_user_ExpectAndReturn(1, 0);

  handleAssociationGetnodeList_ExpectAndReturn(1, 0, NULL, NULL, NODE_LIST_STATUS_SUCCESS);
  handleAssociationGetnodeList_ReturnArrayThruPtr_ppListOfNodes(&pNodeList, listLen);
  handleAssociationGetnodeList_ReturnThruPtr_pListLen(&listLen);
  handleAssociationGetnodeList_IgnoreArg_ppListOfNodes();
  handleAssociationGetnodeList_IgnoreArg_pListLen();
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAnyArgsAndReturn(true);

  // Process command
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, NULL, 0);

  // Verify outgoing frame of the first USER
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The USER_SET for the first user was not answered."
    );

  //------------------
  // Create incoming frame of the second USER (UUID = 2)
  command_handler_input_t input2;
  test_common_clear_command_handler_input(&input2);
  ZW_USER_SET_1BYTE_FRAME incomingFrame2 = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_SET,
    .properties1 = USER_SET_OPERATION_TYPE_ADD,
    .userUniqueIdentifier1 = 0x00,
    .userUniqueIdentifier2 = 0x02,   // User Unique Identifier = 2
    .userType = USER_TYPE_GENERAL,
    .properties2 = 0x00,   // User is not Active
    .credentialRule = CREDENTIAL_RULE_SINGLE,
    .expiringTimeoutMinutes1 = 0x00,
    .expiringTimeoutMinutes2 = 0x00,   // Expiring timout = 0
    .properties3 = USER_NAME_ENCODING_STANDARD_ASCII,
    .userNameLength = 0,   // User Name length = 0, generate Default Name
    .userName1 = 0x00   // First byte of User Name (ignored)
  };
  input2.frame.as_zw_application_tx_buffer.ZW_UserSet1byteFrame
    = incomingFrame2;
  input2.rxOptions.sourceNode.nodeId = 1;
  input2.rxOptions.destNode.nodeId = 2;

  // Set up mock calls
  cc_user_credential_get_max_user_unique_idenfitiers_IgnoreAndReturn(10); // Don't care, just needs to be bigger than the user_unique_identifier
  cc_user_credential_get_max_length_of_user_name_IgnoreAndReturn(40); // Ignore the max name length
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_GENERAL, true);
  cc_user_credential_is_credential_rule_supported_ExpectAndReturn(CREDENTIAL_RULE_SINGLE, true);

  // Mock the call to the user credential add user
  CC_UserCredential_add_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_next_user_ExpectAndReturn(2, 0);

  handleAssociationGetnodeList_ExpectAndReturn(1, 0, NULL, NULL, NODE_LIST_STATUS_SUCCESS);
  handleAssociationGetnodeList_ReturnArrayThruPtr_ppListOfNodes(&pNodeList, listLen);
  handleAssociationGetnodeList_ReturnThruPtr_pListLen(&listLen);
  handleAssociationGetnodeList_IgnoreArg_ppListOfNodes();
  handleAssociationGetnodeList_IgnoreArg_pListLen();
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectAnyArgsAndReturn(true);

  // Process command
  received_frame_status_t status2 =
    invoke_cc_handler_v2(&input2.rxOptions,
                         &input2.frame.as_zw_application_tx_buffer,
                         input2.frameLength, NULL, 0);

  // Verify outgoing frame of the second USER
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status2,
    "The USER_SET for the second user was not answered."
    );

  //------------------
  // Create incoming frame of the USER_REMOVE
  command_handler_input_t input3;
  test_common_clear_command_handler_input(&input3);
  ZW_USER_SET_1BYTE_FRAME incomingFrame_remove = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_SET,
    .properties1 = USER_SET_OPERATION_TYPE_DELETE,
    .userUniqueIdentifier1 = 0x00,
    .userUniqueIdentifier2 = 0x00,   // User Unique Identifier = 0 to indcate "remove all users"
    .userType = USER_TYPE_GENERAL,
    .properties2 = 0x00,   // User is not Active
    .expiringTimeoutMinutes1 = 0x00,
    .expiringTimeoutMinutes2 = 0x00,   // Expiring timout = 0
    .properties3 = USER_NAME_ENCODING_STANDARD_ASCII,
    .userNameLength = 0,   // User Name length = 0, generate Default Name
    .userName1 = 0x00   // First byte of User Name (ignored)
  };
  input3.frame.as_zw_application_tx_buffer.ZW_UserSet1byteFrame
    = incomingFrame_remove;
  input3.rxOptions.sourceNode.nodeId = 1;
  input3.rxOptions.destNode.nodeId = 2;

  // Create expected report
  ZW_USER_REPORT_1BYTE_FRAME expected_report = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = USER_REPORT,
    .userReportType = USER_REP_TYPE_DELETED,
    .nextUserUniqueIdentifier1 = 0,
    .nextUserUniqueIdentifier2 = 0,
    .userModifierType = MODIFIER_TYPE_DNE,
    .userModifierNodeId1 = 0,
    .userModifierNodeId2 = 0,
    .userUniqueIdentifier1 = 0,
    .userUniqueIdentifier2 = 0,
    .userType = 0,
    .properties1 = 0 & USER_REPORT_PROPERTIES1_USER_ACTIVE_STATE_BIT_MASK,
    .credentialRule = CREDENTIAL_RULE_SINGLE,
    .expiringTimeoutMinutes1 = 0,
    .expiringTimeoutMinutes2 = 0,
    .properties2 = USER_NAME_ENCODING_STANDARD_ASCII & USER_REPORT_PROPERTIES2_USER_NAME_ENCODING_MASK,
    .userNameLength = 0,
    .userName1 = 0 // This byte won't appear in the actual frame
  };

  CC_UserCredential_get_next_user_ExpectAndReturn(0, 1); // get the first available user ID
  CC_UserCredential_get_next_user_ExpectAndReturn(1, 2); // get user ID after the first one
  CC_UserCredential_get_next_user_IgnoreAndReturn(0); // no more users

  CC_UserCredential_get_next_credential_IgnoreAndReturn(0);
  CC_UserCredential_delete_credential_IgnoreAndReturn(1);

  CC_UserCredential_delete_user_ExpectAndReturn(1, U3C_DB_OPERATION_RESULT_SUCCESS); // delete the first user
  CC_UserCredential_delete_user_ExpectAndReturn(2, U3C_DB_OPERATION_RESULT_SUCCESS); // delete the second user

  handleAssociationGetnodeList_ExpectAndReturn(1, 0, NULL, NULL, NODE_LIST_STATUS_SUCCESS);
  handleAssociationGetnodeList_ReturnArrayThruPtr_ppListOfNodes(&pNodeList, listLen);
  handleAssociationGetnodeList_ReturnThruPtr_pListLen(&listLen);
  handleAssociationGetnodeList_IgnoreArg_ppListOfNodes();
  handleAssociationGetnodeList_IgnoreArg_pListLen();

  zaf_transport_tx_StopIgnore();

  // Verify report
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_ExpectWithArrayAndReturn((uint8_t *)&expected_report, 1, sizeof(expected_report) - 1, NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status3 =
    invoke_cc_handler_v2(&input3.rxOptions,
                         &input3.frame.as_zw_application_tx_buffer,
                         input3.frameLength, NULL, 0);

  // Verify outgoing frame of the second USER
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status3,
    "The USER_SET remove all users command was not answered."
    );

  /**
   *  While calling Expect_...() resets this, the calls can be completely
   *  left out and the code will pass.
   */
  CC_UserCredential_get_next_user_StopIgnore();
}
