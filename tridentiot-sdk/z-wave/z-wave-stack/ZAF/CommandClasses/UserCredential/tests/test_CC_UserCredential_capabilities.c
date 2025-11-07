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
 * Verifies that the End Device responds correctly to User Capabilities Get.
 */
void test_USER_CREDENTIAL_user_capabilities_report(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_USER_CAPABILITIES_GET_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    USER_CAPABILITIES_GET
  };
  input.frame.as_zw_application_tx_buffer.ZW_UserCapabilitiesGetFrame
    = incomingFrame;

  // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_UserCapabilitiesReport2byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = USER_CAPABILITIES_REPORT,
      .numberOfSupportedUserUniqueIdentifiers1 = 0xFA,
      .numberOfSupportedUserUniqueIdentifiers2 = 0xF9,
      .supportedCredentialRulesBitMask = 0x0A,
      .maxLengthOfUserName = 188,
      // #TODO: Change when User Schedule is implemented
      .properties1 = 0x60,
      .supportedUserTypesBitMaskLength = 2,
      .variantgroup1.supportedUserTypesBitMask = 0x29,
      .variantgroup2.supportedUserTypesBitMask = 0x02
    }
  };

  // Set up mock calls
  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(0xFAF9);
  cc_user_credential_get_max_length_of_user_name_ExpectAndReturn(188);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_GENERAL, true);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_PROGRAMMING, true);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_NON_ACCESS, false);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_DURESS, true);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_DISPOSABLE, false);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_EXPIRING, false);
  cc_user_credential_is_user_type_supported_ExpectAndReturn(USER_TYPE_REMOTE_ONLY, true);
  cc_user_credential_is_credential_rule_supported_ExpectAndReturn(CREDENTIAL_RULE_SINGLE, true);
  cc_user_credential_is_credential_rule_supported_ExpectAndReturn(CREDENTIAL_RULE_DUAL, false);
  cc_user_credential_is_credential_rule_supported_ExpectAndReturn(CREDENTIAL_RULE_TRIPLE, true);
  cc_user_credential_is_all_users_checksum_supported_ExpectAndReturn(true);
  cc_user_credential_is_user_checksum_supported_ExpectAndReturn(true);

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
    "The User Capabilities Get was not answered."
    );
  TEST_ASSERT_EQUAL_MESSAGE(
    sizeof(ZW_USER_CAPABILITIES_REPORT_2BYTE_FRAME), lengthOut,
    "The outgoing frame was not the right size."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output,
    sizeof(ZW_USER_CAPABILITIES_REPORT_2BYTE_FRAME),
    "The outgoing frame had unexpected contents."
    );
}

/*
 * Verifies that the End Device responds correctly to Credential Capabilities Get.
 */
void test_USER_CREDENTIAL_credential_capabilities_report(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_USER_CAPABILITIES_GET_FRAME incomingFrame = {
    COMMAND_CLASS_USER_CREDENTIAL,
    CREDENTIAL_CAPABILITIES_GET
  };
  input.frame.as_zw_application_tx_buffer.ZW_UserCapabilitiesGetFrame
    = incomingFrame;

  // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_CredentialCapabilitiesReport4byteFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = CREDENTIAL_CAPABILITIES_REPORT,
      .properties1 = (1 << 7) | (1 << 6), // Credential checksum is supported, AC is supported
      .numberOfSupportedCredentialTypes = 5,
    }
  };
  uint8_t * vg_offset = (uint8_t *)&expectedOutput.ZW_CredentialCapabilitiesReport1byteFrame.variantgroup1;
  uint8_t vg_byte_array[] = {
    // Credential Type (1 byte)
    CREDENTIAL_TYPE_PIN_CODE, CREDENTIAL_TYPE_PASSWORD, CREDENTIAL_TYPE_NFC, CREDENTIAL_TYPE_FINGER_BIOMETRIC, CREDENTIAL_TYPE_HAND_BIOMETRIC,
    // Properties2 (1 byte, CL Support = bit 7)
    0x80, 0x80, 0x00, 0x00, 0x00,
    // Number of Supported Credential Slots (2 bytes)
    0x3F, 0x1A, 0xC1, 0xDE, 0x66, 0xA0, 0x60, 0xA7, 0x0C, 0xA1,
    // Min Length of Credential Data
    0xC3, 0x10, 0x21, 0x1F, 0xBD,
    // Max Length of Credential Data
    0x8F, 0x22, 0xB9, 0x37, 0x4B,
    // Credential Learn Recommended Timeout
    0xFA, 0x7B, 0x2D, 0x10, 0x1D,
    // Credential Learn Number of Steps
    0xFF, 0x91, 0x73, 0x72, 0xED,
    // Max Hash Length
    0x11, 0x42, 0x8F, 0xBA, 0xBF
  };
  memcpy(vg_offset, vg_byte_array, sizeof(vg_byte_array));
  uint8_t frame_size = 4 + sizeof(vg_byte_array);

  // Set up mock calls
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(true);
  cc_user_credential_get_number_of_supported_credential_types_ExpectAndReturn(5);
  cc_user_credential_get_admin_code_supported_ExpectAndReturn(true);
  cc_user_credential_get_admin_code_deactivate_supported_ExpectAndReturn(false);

  bool supported_types[] = { 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0 };
  bool cl_support[] =     { 1, 1, 0, 0, 0 };
  uint8_t max_hash_length[] = { 0x11, 0x42, 0x8F, 0xBA, 0xBF };
  uint16_t max_slots[] =  { 16154, 49630, 26272, 24743, 3233 };
  uint8_t min_length[] = { 0xC3, 0x10, 0x21, 0x1F, 0xBD };
  uint8_t max_length[] = { 0x8F, 0x22, 0xB9, 0x37, 0x4B };
  uint8_t cl_timeout[] = { 250, 123, 45, 16, 29 };
  uint8_t cl_steps[] = { 255, 145, 115, 114, 237 };
  uint8_t i_supported = 0;

  for (uint8_t i = 1; i <= CREDENTIAL_TYPE_UNSPECIFIED_BIOMETRIC; ++i) {
    bool supported = supported_types[i];
    cc_user_credential_is_credential_type_supported_ExpectAndReturn(i, supported);
    if (supported) {
      cc_user_credential_is_credential_learn_supported_ExpectAndReturn(i, cl_support[i_supported]);
      cc_user_credential_get_max_credential_slots_ExpectAndReturn(i, max_slots[i_supported]);
      cc_user_credential_get_min_length_of_data_ExpectAndReturn(i, min_length[i_supported]);
      cc_user_credential_get_max_length_of_data_ExpectAndReturn(i, max_length[i_supported]);
      cc_user_credential_get_cl_recommended_timeout_ExpectAndReturn(i, cl_timeout[i_supported]);
      cc_user_credential_get_cl_number_of_steps_ExpectAndReturn(i, cl_steps[i_supported]);
      cc_user_credential_get_max_hash_length_ExpectAndReturn(i, max_hash_length[i_supported]);
      ++i_supported;
    }
  }

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
    "The Credential Capabilities Get was not answered."
    );
  TEST_ASSERT_EQUAL_MESSAGE(
    frame_size, lengthOut, "The outgoing frame was not the right size."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output, frame_size,
    "The outgoing frame had unexpected contents."
    );
}
