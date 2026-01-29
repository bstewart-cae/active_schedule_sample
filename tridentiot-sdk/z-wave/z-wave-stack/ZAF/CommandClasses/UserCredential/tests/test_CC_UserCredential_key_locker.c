/**
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Card Access Engineering LLC
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

/*
 * Verifies that the End Device responds correctly to Key Locker Capabilities Get.
 */
void test_USER_CREDENTIAL_key_locker_capabilities_report(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_CAPABILITIES_GET_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_CAPABILITIES_GET_V2,
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerCapabilitiesGetV2Frame
    = incomingFrame;

  // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_KeyLockerCapabilitiesReport1byteV2Frame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = KEY_LOCKER_CAPABILITIES_REPORT_V2,
      .numberOfSupportedEntryTypes = 0x01,
      .variantgroup1.supportedEntryType = KEY_LOCKER_CAPABILITIES_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
      .variantgroup1_1.numberOfEntrySlots1 = 0x00,
      .variantgroup1_1.numberOfEntrySlots2 = 0x0A,
      .variantgroup1_2.minLengthOfEntryData1 = 0x00,
      .variantgroup1_2.minLengthOfEntryData2 = 0x03,
      .variantgroup1_3.maxLengthOfEntryData1 = 0x00,
      .variantgroup1_3.maxLengthOfEntryData2 = 0xFF,
    }
  };

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x0A);
  cc_user_credential_get_key_locker_min_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x03);
  cc_user_credential_get_key_locker_max_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0xFF);

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Key Locker Capabilities Get was not answered."
    );
  TEST_ASSERT_EQUAL_MESSAGE(
    sizeof(ZW_KEY_LOCKER_CAPABILITIES_REPORT_1BYTE_V2_FRAME), lengthOut,
    "The outgoing frame was not the right size."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output,
    sizeof(ZW_KEY_LOCKER_CAPABILITIES_REPORT_1BYTE_V2_FRAME),
    "The outgoing frame had unexpected contents."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

/*
 * Verifies that the End Device responds correctly to Key Locker Capabilities Get if Key Locker is not supported
 */
void test_USER_CREDENTIAL_key_locker_capabilities_report_no_types_supported(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_CAPABILITIES_GET_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_CAPABILITIES_GET_V2,
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerCapabilitiesGetV2Frame
    = incomingFrame;

    // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_KeyLockerCapabilitiesReport1byteV2Frame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = KEY_LOCKER_CAPABILITIES_REPORT_V2,
      .numberOfSupportedEntryTypes = 0x00,
    }
  };
  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x00);

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Key Locker Capabilities Get was not answered."
    );
  TEST_ASSERT_EQUAL_MESSAGE(
    KEY_LOCKER_CAP_REPORT_VG_OFFSET, lengthOut,
    "The outgoing frame was not the right size."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output,
    KEY_LOCKER_CAP_REPORT_VG_OFFSET,
    "The outgoing frame had unexpected contents."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

/*
 * Verifies that the End Device ignores a Key Locker get operation of an unsupported type
 */
void test_USER_CREDENTIAL_key_locker_get_invalid_type(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_GET_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_GET_V2,
    .entryType = KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    .entrySlot1 = 0x00,
    .entrySlot2 = 0x01,
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntryGetV2Frame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x00);

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Key Locker Capabilities Get was answered."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

/*
 * Verifies that the End Device ignores a Key Locker get operation for an invalid slot (0) of a supported type
 */
void test_USER_CREDENTIAL_key_locker_get_invalid_slot_zero(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_GET_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_GET_V2,
    KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    0x00,
    0x00
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntryGetV2Frame
    = incomingFrame;

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Key Locker Capabilities Get was answered."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

/*
 * Verifies that the End Device ignores a Key Locker get operation for an invalid slot (> 0)of a supported type
 */
void test_USER_CREDENTIAL_key_locker_get_invalid_slot_non_zero(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_GET_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_GET_V2,
    KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    0x00,
    0x11
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntryGetV2Frame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x10);

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Key Locker Capabilities Get was answered."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

u3c_io_operation_status_t GetKeyLockerData_callback(
  u3c_kl_get_input_t* inputs,
  u3c_kl_get_output_t* outputs,
  __attribute__((unused)) int cmock_num_calls)
{
  switch (inputs->slot) {
    case 0x01: { // Empty
      TEST_ASSERT_EQUAL_MESSAGE(
        inputs->slot_type, U3C_KL_SLOT_TYPE_DESFIRE,
        "The Key Locker Get input slot type is malformed."
        );
      // Populate output information
      outputs->occupied = false;
      outputs->slot = inputs->slot;
      outputs->slot_type = inputs->slot_type;
      // Return
      return (u3c_io_operation_status_t) {
        .result = U3C_DB_OPERATION_RESULT_SUCCESS,
        .working_time = 0
      };
    }
    case 0x09: // Full
      TEST_ASSERT_EQUAL_MESSAGE(
        inputs->slot_type, U3C_KL_SLOT_TYPE_DESFIRE,
        "The Key Locker Get input slot type is malformed."
        );
      // Populate output information
      outputs->occupied = true;
      outputs->slot = inputs->slot;
      outputs->slot_type = inputs->slot_type;
      // Return
      return (u3c_io_operation_status_t) {
        .result = U3C_DB_OPERATION_RESULT_SUCCESS,
        .working_time = 0
      };
    case 0x08: // Working (request not processed instantly)
      TEST_ASSERT_EQUAL_MESSAGE(
        inputs->slot_type, U3C_KL_SLOT_TYPE_DESFIRE,
        "The Key Locker Get input slot type is malformed."
        );
      // Return
      return (u3c_io_operation_status_t) {
        .result = U3C_DB_OPERATION_RESULT_WORKING,
        .working_time = 10
      };
    default:
      TEST_ASSERT_EQUAL_MESSAGE(
        inputs->slot, 0xFF,
        "Get Key Locker called with invalid slot #"
      );
      return (u3c_io_operation_status_t) {
        .result = RECEIVED_FRAME_STATUS_FAIL,
        .working_time = 0
      };
  }
}

/*
 * Verifies that the End Device processes a Key Locker Get operation appropriately
 */
void test_USER_CREDENTIAL_key_locker_entry_get(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_GET_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_GET_V2,
    KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    0x00,
    0x01
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntryGetV2Frame
    = incomingFrame;


  // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_KeyLockerEntryReportV2Frame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = KEY_LOCKER_ENTRY_REPORT_V2,
      .properties1 = 0x00,
      .entryType = KEY_LOCKER_ENTRY_GET_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
      .entrySlot1 = 0x00,
      .entrySlot2 = 0x01,
    }
  };

  // Create expected outgoing frame (for a full one)
  ZW_APPLICATION_TX_BUFFER expectedOutput2 = {
    .ZW_KeyLockerEntryReportV2Frame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = KEY_LOCKER_ENTRY_REPORT_V2,
      .properties1 = 0x01,
      .entryType = KEY_LOCKER_ENTRY_GET_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
      .entrySlot1 = 0x00,
      .entrySlot2 = 0x09,
    }
  };

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x10);
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x10);
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x10);
  CC_UserCredential_get_key_locker_entry_StubWithCallback(GetKeyLockerData_callback);

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Key Locker Get was not answered."
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    sizeof(ZW_KEY_LOCKER_ENTRY_REPORT_V2_FRAME), lengthOut,
    "The outgoing frame was not the right size."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output,
    sizeof(ZW_KEY_LOCKER_ENTRY_REPORT_V2_FRAME),
    "The outgoing (empty) frame had unexpected contents."
    );

   /*
    * Run command again, but return a filled slot (slot 9) this time
    */
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntryGetV2Frame.entrySlot2 = 0x09;
  status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);
  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Key Locker Get was not answered."
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    sizeof(ZW_KEY_LOCKER_ENTRY_REPORT_V2_FRAME), lengthOut,
    "The outgoing frame was not the right size."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput2, &output,
    sizeof(ZW_KEY_LOCKER_ENTRY_REPORT_V2_FRAME),
    "The outgoing (full) frame had unexpected contents."
    );

  /*
   * Run command again, but return a working slot (slot 8) this time
   */
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntryGetV2Frame.entrySlot2 = 0x08;
  status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);
  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_WORKING, status,
    "The Key Locker Get was not answered correctly."
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    0, lengthOut,
    "The outgoing frame was not the right size."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
  // Reset Stub
  CC_UserCredential_get_key_locker_entry_StubWithCallback(NULL);
}

/*
 * Verifies that the End Device processes a Key Locker Set operation for an unsupported key type appropriately
 */
void test_USER_CREDENTIAL_key_locker_entry_set_unsupported(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_SET_4BYTE_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_SET_V2,
    KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    0x00, // Slot MSB
    0x01, // Slot LSB
    KEY_LOCKER_ENTRY_SET_OPERATION_TYPE_ADD_V2,
    0x00, // Length MSB
    0x04, // Length LSB
    0x01,
    0x02,
    0x03,
    0x04,
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntrySet4byteV2Frame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x00);

  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Key Locker Get was not answered correctly."
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    0, lengthOut,
    "The outgoing frame was not the right size."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

/*
 * Verifies that the End Device processes a Key Locker Set operation with an invalid data length appropriately
 */
void test_USER_CREDENTIAL_key_locker_entry_set_bad_data_length(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_SET_4BYTE_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_SET_V2,
    KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    0x00, // Slot MSB
    0x01, // Slot LSB
    KEY_LOCKER_ENTRY_SET_OPERATION_TYPE_ADD_V2,
    0x00, // Length MSB
    0x04, // Length LSB
    0x01,
    0x02,
    0x03,
    0x04,
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntrySet4byteV2Frame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x03);
  cc_user_credential_get_key_locker_min_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x05); // Test too short
  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Key Locker Set was answered and should not have been."
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    0, lengthOut,
    "The outgoing frame was not the right size."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x03);
  cc_user_credential_get_key_locker_max_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x03);
  cc_user_credential_get_key_locker_min_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x00); // Test too long
  // Process command
  lengthOut = 0;
  status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Key Locker Capabilities Get was not answered correctly."
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    0, lengthOut,
    "The outgoing frame was not the right size."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
}

u3c_io_operation_status_t key_locker_set(
  u3c_kl_set_input_t* inputs,
  __attribute__((unused)) u3c_kl_set_output_t* outputs,
  __attribute__((unused)) int cmock_num_calls)
{
  TEST_ASSERT_NOT_EQUAL(NULL, inputs->rx_opts);
  TEST_ASSERT_NOT_EQUAL(NULL, inputs->data);
  TEST_ASSERT_EQUAL_MESSAGE(
    inputs->length, 4,
    "invalid length extracted"
    );
  TEST_ASSERT_EQUAL_MESSAGE(
    inputs->slot, 1,
    "invalid slot extracted"
    );
  TEST_ASSERT_EQUAL_MESSAGE(
    inputs->slot_type, U3C_KL_SLOT_TYPE_DESFIRE,
    "invalid slot type extracted"
    );
  return (u3c_io_operation_status_t) {
    .result = U3C_DB_OPERATION_RESULT_SUCCESS,
    .working_time = 0
  };
}

/*
 * Verifies that the End Device processes a valid Key Locker Set operation appropriately
 */
void test_USER_CREDENTIAL_key_locker_entry_set_valid(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_KEY_LOCKER_ENTRY_SET_4BYTE_V2_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    KEY_LOCKER_ENTRY_SET_V2,
    KEY_LOCKER_ENTRY_REPORT_DESFIRE_EV2_3_APPLICATION_ID_KEY_V2,
    0x00, // Slot MSB
    0x01, // Slot LSB
    KEY_LOCKER_ENTRY_SET_OPERATION_TYPE_ADD_V2,
    0x00, // Length MSB
    0x04, // Length LSB
    0x01,
    0x02,
    0x03,
    0x04,
  };
  input.frameLength = sizeof(incomingFrame);
  input.frame.as_zw_application_tx_buffer.ZW_KeyLockerEntrySet4byteV2Frame
    = incomingFrame;

  // Set up mock calls
  cc_user_credential_get_key_locker_slot_count_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x03);
  cc_user_credential_get_key_locker_max_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x05);
  cc_user_credential_get_key_locker_min_data_length_ExpectAndReturn(U3C_KL_SLOT_TYPE_DESFIRE, 0x03);

  CC_UserCredential_set_key_locker_entry_StubWithCallback(key_locker_set);
  // Process command
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Key Locker Set was not answered"
    );
   TEST_ASSERT_EQUAL_MESSAGE(
    0, lengthOut,
    "The outgoing frame was not the right size."
    );

  cc_user_credential_io_mock_Verify();
  cc_user_credential_config_api_mock_Verify();
  CC_UserCredential_set_key_locker_entry_StubWithCallback(NULL);
}
