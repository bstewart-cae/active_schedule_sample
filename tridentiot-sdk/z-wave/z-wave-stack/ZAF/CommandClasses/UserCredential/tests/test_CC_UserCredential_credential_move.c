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
#include "cc_user_credential_config_api_mock.h"
#include "cc_user_credential_io_mock.h"
#include "zaf_transport_tx_mock.h"
#include "SwTimer_mock.h"
#include "SizeOf.h"

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

/**
 * Verifies that a User Unique Identifier Credential Association Set command
 * is accepted when the arguments are correct and rejected when:
 * - The Source Credential Type is not supported
 * - The Source Credential Slot is zero
 * - The Source Credential Slot is too large
 * - The Source Credential Slot is empty
 * - The Destination User Unique Identifier is zero
 * - The Destination User Unique Identifier is too large
 * - The Destination User Unique Identifier does not exist
 * - The Destination Credential Slot is zero
 * - The Destination Credential Slot is too large
 * - The Destination Credential Slot is occupied
 */
void test_USER_CREDENTIAL_uuid_credential_association_set(void)
{
  typedef enum test_case_ {
    TC_CONTROL,
    TC_TYPE_INVALID,
    TC_SRC_SLOT_ZERO,
    TC_SRC_SLOT_TOO_LARGE,
    TC_SRC_SLOT_EMPTY,
    TC_DST_UUID_ZERO,
    TC_DST_UUID_TOO_LARGE,
    TC_DST_UUID_DNE,
    TC_DST_SLOT_ZERO,
    TC_DST_SLOT_TOO_LARGE,
    TC_DST_SLOT_OCCUPIED,
    NUMBER_OF_TEST_CASES
  } test_case;

  // The last function to mock before we expect the unit to return
  typedef enum last_mock_ {
    LM_IS_TYPE_SUPPORTED,
    LM_MAX_SLOTS_SRC,
    LM_MAX_UUID,
    LM_MAX_SLOTS_DST,
    LM_GET_USER_DST,
    LM_MOVE
  } last_mock;

  for (test_case tc = 0; tc < NUMBER_OF_TEST_CASES; ++tc) {
    // Arguments of incoming frame
    u3c_credential_type type = CREDENTIAL_TYPE_EYE_BIOMETRIC;
    uint16_t dst_uuid = 83;
    uint16_t src_slot = 34;
    uint16_t dst_slot = 106;

    // Values to be returned by mocked functions
    last_mock last_mock = LM_MOVE;
    bool supports_type = true;
    uint16_t max_uuids = 200;
    uint16_t max_slots = 200;
    u3c_db_operation_result dst_user_get_res = U3C_DB_OPERATION_RESULT_SUCCESS;
    u3c_db_operation_result db_move_res = U3C_DB_OPERATION_RESULT_SUCCESS;

    // Expectations
    received_frame_status_t expected_frame_status = RECEIVED_FRAME_STATUS_FAIL;
    uint8_t expected_status = UINT8_MAX;

    switch (tc) {
      case TC_CONTROL: {
        expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_SUCCESS;
        break;
      }
      case TC_TYPE_INVALID: {
        supports_type = false;
        last_mock = LM_IS_TYPE_SUPPORTED;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_CREDENTIAL_TYPE_INVALID;
        break;
      }
      case TC_SRC_SLOT_ZERO: {
        src_slot = 0;
        last_mock = LM_MAX_SLOTS_SRC;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_SOURCE_CREDENTIAL_SLOT_INVALID;
        break;
      }
      case TC_SRC_SLOT_TOO_LARGE: {
        src_slot = 973;
        last_mock = LM_MAX_SLOTS_SRC;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_SOURCE_CREDENTIAL_SLOT_INVALID;
        break;
      }
      case TC_SRC_SLOT_EMPTY: {
        db_move_res = U3C_DB_OPERATION_RESULT_FAIL_DNE;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_SOURCE_CREDENTIAL_SLOT_EMPTY;
        break;
      }
      case TC_DST_UUID_ZERO: {
        dst_uuid = 0;
        last_mock = LM_MAX_UUID;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_DESTINATION_USER_UNIQUE_IDENTIFIER_INVALID;
        break;
      }
      case TC_DST_UUID_TOO_LARGE: {
        dst_uuid = 213;
        last_mock = LM_MAX_UUID;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_DESTINATION_USER_UNIQUE_IDENTIFIER_INVALID;
        break;
      }
      case TC_DST_UUID_DNE: {
        last_mock = LM_GET_USER_DST;
        dst_user_get_res = U3C_DB_OPERATION_RESULT_FAIL_DNE;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_DESTINATION_USER_UNIQUE_IDENTIFIER_NONEXISTENT;
        break;
      }
      case TC_DST_SLOT_ZERO: {
        dst_slot = 0;
        last_mock = LM_MAX_UUID;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_DESTINATION_CREDENTIAL_SLOT_INVALID;
        break;
      }
      case TC_DST_SLOT_TOO_LARGE: {
        dst_slot = 0x173E;
        last_mock = LM_MAX_UUID;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_DESTINATION_CREDENTIAL_SLOT_INVALID;
        break;
      }
      case TC_DST_SLOT_OCCUPIED: {
        db_move_res = U3C_DB_OPERATION_RESULT_FAIL_OCCUPIED;
        expected_status = USER_CREDENTIAL_ASSOCIATION_REPORT_DESTINATION_CREDENTIAL_SLOT_OCCUPIED;
        break;
      }
      default: {
        break;
      }
    }

    // Create incoming frame
    command_handler_input_t input;
    test_common_clear_command_handler_input(&input);
    ZW_USER_CREDENTIAL_ASSOCIATION_SET_FRAME incomingFrame = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = USER_CREDENTIAL_ASSOCIATION_SET,
      .credentialType = type,
      .sourceCredentialSlot1 = src_slot >> 8,
      .sourceCredentialSlot2 = src_slot & 0xFF,
      .destinationUserUniqueIdentifier1 = dst_uuid >> 8,
      .destinationUserUniqueIdentifier2 = dst_uuid & 0xFF,
      .destinationCredentialSlot1 = dst_slot >> 8,
      .destinationCredentialSlot2 = dst_slot & 0xFF
    };
    // u3c_credential_metadata_t source_metadata = {
    //   .uuid = src_uuid,
    //   .slot = src_slot,
    //   .type = type,
    // };
    input.frame.as_zw_application_tx_buffer.ZW_UserCredentialAssociationSetFrame = incomingFrame;
    input.rxOptions.sourceNode.nodeId = 1;
    input.rxOptions.destNode.nodeId = 2;
    ZW_APPLICATION_TX_BUFFER output;
    uint8_t length_out = 0;

    ZW_USER_CREDENTIAL_ASSOCIATION_REPORT_FRAME report = {
      .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
      .cmd = USER_CREDENTIAL_ASSOCIATION_REPORT,
      .credentialType = type,
      .sourceCredentialSlot1 = src_slot >> 8,
      .sourceCredentialSlot2 = src_slot & 0xFF,
      .destinationUserUniqueIdentifier1 = dst_uuid >> 8,
      .destinationUserUniqueIdentifier2 = dst_uuid & 0xFF,
      .destinationCredentialSlot1 = dst_slot >> 8,
      .destinationCredentialSlot2 = dst_slot & 0xFF,
      .userCredentialAssociationStatus = expected_status
    };

    // Set up relevant mock calls

    if (last_mock >= LM_IS_TYPE_SUPPORTED) {
      cc_user_credential_is_credential_type_supported_ExpectAndReturn(type, supports_type);
    }
    if (last_mock >= LM_MAX_SLOTS_SRC) {
      cc_user_credential_get_max_credential_slots_ExpectAndReturn(type, max_slots);
    }
    if (last_mock >= LM_MAX_UUID) {
      cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(max_uuids);
    }
    if (last_mock >= LM_MAX_SLOTS_DST) {
      cc_user_credential_get_max_credential_slots_ExpectAndReturn(type, max_slots);
    }
    if (last_mock >= LM_GET_USER_DST) {
      CC_UserCredential_get_user_ExpectAndReturn(dst_uuid, NULL, NULL, dst_user_get_res);
      CC_UserCredential_get_user_IgnoreArg_user();
      CC_UserCredential_get_user_IgnoreArg_name();
    }
    // CC_UserCredential_get_credential_ExpectAndReturn(src_uuid, type, src_slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
    // CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&source_metadata);
    // CC_UserCredential_get_credential_IgnoreArg_user_unique_identifier();
    // CC_UserCredential_get_credential_IgnoreArg_credential_type();
    // CC_UserCredential_get_credential_IgnoreArg_credential_slot();
    // CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
    // CC_UserCredential_get_credential_IgnoreArg_credential_data();
    if (last_mock >= LM_MOVE) {
      CC_UserCredential_move_credential_ExpectAndReturn(type, src_slot, dst_uuid, dst_slot, db_move_res);
    }

    zaf_transport_rx_to_tx_options_Ignore();
    zaf_transport_tx_ExpectWithArrayAndReturn((uint8_t *)&report, 1, sizeof(report), NULL, NULL, 0, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    // Process command
    received_frame_status_t status =
      invoke_cc_handler_v2(&input.rxOptions,
                           &input.frame.as_zw_application_tx_buffer,
                           input.frameLength, &output, &length_out);

    // Verify outgoing frame
    TEST_ASSERT_EQUAL_MESSAGE(
      expected_frame_status, status,
      "[UUID Association] Handler returned invalid frame status!"
      );
  }
}
