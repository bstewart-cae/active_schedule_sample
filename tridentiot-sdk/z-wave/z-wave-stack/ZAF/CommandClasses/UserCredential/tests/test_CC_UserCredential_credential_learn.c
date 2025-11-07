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
#include "ZAF_Common_interface_mock.h"
#include "zaf_event_distributor_soc_mock.h"
#include "zaf_transport_tx_mock.h"
#include "SwTimer_mock.h"
#include "SizeOf.h"
#include "ZW_TransportSecProtocol_mock.h"
#include "AppTimer_mock.h"
#include "zpal_power_manager_mock.h"

zpal_pm_handle_t pm_handle;

void setUpSuite(void)
{
  ZAF_GetSucNodeId_IgnoreAndReturn(1);
  ZAF_GetNodeID_IgnoreAndReturn(2);
  ZAF_GetSecurityKeys_IgnoreAndReturn(SECURITY_KEY_S2_ACCESS);
  GetHighestSecureLevel_IgnoreAndReturn(SECURITY_KEY_S2_ACCESS);
  AppTimerRegister_IgnoreAndReturn(true);
  zpal_pm_register_IgnoreAndReturn(pm_handle);
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

static void credential_learn_start_create(
  ZW_CREDENTIAL_LEARN_START_FRAME * frame, u3c_event_data_learn_start_t params)
{
  frame->cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
  frame->cmd = CREDENTIAL_LEARN_START,
  frame->userUniqueIdentifier1 = params.target.uuid >> 8;
  frame->userUniqueIdentifier2 = params.target.uuid & 0xFF;
  frame->credentialType = params.target.type;
  frame->credentialSlot1 = params.target.slot >> 8;
  frame->credentialSlot2 = params.target.slot & 0xFF;
  frame->properties1 = params.operation_type & CREDENTIAL_LEARN_START_PROPERTIES1_OPERATION_TYPE_MASK;
  frame->credentialLearnTimeout = params.timeout_seconds;
}

/**
 * Verifies that a Credential Learn Cancel request is honored when the Learn
 * process is in progress.
 */
void test_USER_CREDENTIAL_credential_learn_cancel(void)
{
  // Set up inconsequential mock calls
  cc_user_credential_config_api_mock_Init();
  zaf_transport_rx_to_tx_options_Ignore();
  zaf_transport_tx_IgnoreAndReturn(true);
  zaf_event_distributor_enqueue_cc_event_IgnoreAndReturn(true);

  // Set up inputs and outputs of the command handler
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  input.rxOptions.sourceNode.nodeId = 1;
  input.rxOptions.destNode.nodeId = 2;
  ZW_CREDENTIAL_LEARN_START_FRAME cl_start_frame;
  u3c_event_data_learn_start_t params = {
    .operation_type = U3C_OPERATION_TYPE_ADD,
    .target = {
      .uuid = 1,
      .type = CREDENTIAL_TYPE_PIN_CODE,
      .slot = 1,
    },
    .timeout_seconds = 12
  };
  credential_learn_start_create(&cl_start_frame, params);
  input.frame.as_zw_application_tx_buffer.ZW_CredentialLearnStartFrame = cl_start_frame;
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;

  // Set up mock calls
  cc_user_credential_get_max_credential_slots_IgnoreAndReturn(999);
  cc_user_credential_is_credential_type_supported_IgnoreAndReturn(true);
  cc_user_credential_get_max_user_unique_idenfitiers_IgnoreAndReturn(999);
  cc_user_credential_get_max_credential_slots_IgnoreAndReturn(999);
  cc_user_credential_is_credential_learn_supported_IgnoreAndReturn(true);
  CC_UserCredential_get_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_FAIL_DNE);

  // Process command
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame status
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Credential Learn process could not be started."
    );

  // Set up inputs and outputs of the command handler
  test_common_clear_command_handler_input(&input);
  input.rxOptions.sourceNode.nodeId = 1;
  input.rxOptions.destNode.nodeId = 2;
  ZW_CREDENTIAL_LEARN_CANCEL_FRAME cl_cancel_frame = {
    .cmdClass = COMMAND_CLASS_USER_CREDENTIAL,
    .cmd = CREDENTIAL_LEARN_CANCEL
  };
  input.frame.as_zw_application_tx_buffer.ZW_CredentialLearnCancelFrame = cl_cancel_frame;
  memset(&output, 0, sizeof(output) * sizeof(uint8_t));
  lengthOut = 0;

  // Set up mock calls
  TimerStop_IgnoreAndReturn(ESWTIMER_STATUS_SUCCESS);
  zpal_pm_cancel_Expect(pm_handle);

  // Process command
  status = invoke_cc_handler_v2(&input.rxOptions,
                                &input.frame.as_zw_application_tx_buffer,
                                input.frameLength, &output, &lengthOut);

  // Verify outgoing frame status
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Credential Learn process was not cancelled."
    );
}

/**
 * Verifies that a Credential Learn Start request
 * - is accepted when the arguments are correct
 * - is ignored in the following cases:
 *   - Credential Type is not supported
 *   - UUID is 0
 *   - UUID is greater than supported
 *   - Credential Slot is 0
 *   - Credential Slot is greater than supported
 *   - Credential Type does not support Credential Learn
 * - is actively rejected when:
 *   - A new Credential would be added to an occupied slot
 *   - An existing Credential would be modified but it is not found
 *   - The Credential Learn process is already in progress
 */
void test_USER_CREDENTIAL_credential_learn_start(void)
{
  typedef enum test_case_ {
    TC_TYPE_NOT_SUPPORTED,             ///< CC:0083.01.0F.11.003
    TC_UUID_ZERO,                      ///< CC:0083.01.05.11.015
    TC_UUID_TOO_LARGE,                 ///< CC:0083.01.0F.11.005
    TC_CREDENTIAL_SLOT_ZERO,           ///< CC:0083.01.0A.11.005
    TC_CREDENTIAL_SLOT_TOO_LARGE,      ///< CC:0083.01.0F.11.006
    TC_CREDENTIAL_LEARN_NOT_SUPPORTED, ///< CC:0083.01.0F.11.004
    TC_ADD_SLOT_OCCUPIED,              ///< CC:0083.01.0F.11.002
    TC_MODIFY_DOES_NOT_EXIST,          ///< CC:0083.01.0F.11.002
    TC_CONTROL,                        ///< Control test case, should succeed
    TC_ALREADY_IN_PROGRESS,            ///< CC:0083.01.11.11.001
    NUMBER_OF_TEST_CASES
  } test_case;

  // Set up inconsequential mock calls
  zaf_event_distributor_enqueue_cc_event_IgnoreAndReturn(true);
  CC_UserCredential_get_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);

  for (test_case tc = 0; tc < NUMBER_OF_TEST_CASES; ++tc) {
    // Clear previous state of config API mocks
    cc_user_credential_config_api_mock_Init();
    zaf_transport_tx_mock_Init();
    zaf_transport_rx_to_tx_options_Ignore();

    // Incoming frame's arguments
    u3c_operation_type_t operation_type = U3C_OPERATION_TYPE_ADD;
    uint16_t uuid = 45419;
    u3c_credential_type type = CREDENTIAL_TYPE_PIN_CODE;
    uint16_t slot = 181;
    uint8_t timeout = 13;

    // Values returned by mocked functions
    bool supports_type = true;
    uint16_t max_uuids = UINT16_MAX;
    uint16_t max_slots = UINT16_MAX;
    bool supports_cl = true;

    // Expectations
    uint8_t steps_remaining = 0;
    received_frame_status_t expected_status = RECEIVED_FRAME_STATUS_FAIL;
    u3c_credential_learn_status_t cl_status = 0;
    bool is_report_expected = false;

    // Modify arguments and set up additional mock calls based on the test case
    switch (tc) {
      case TC_TYPE_NOT_SUPPORTED: {
        expected_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
        supports_type = false;
        break;
      }
      case TC_UUID_ZERO: {
        uuid = 0;
        break;
      }
      case TC_UUID_TOO_LARGE: {
        expected_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
        max_uuids = 5;
        break;
      }
      case TC_CREDENTIAL_SLOT_ZERO: {
        slot = 0;
        break;
      }
      case TC_CREDENTIAL_SLOT_TOO_LARGE: {
        expected_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
        max_slots = 5;
        break;
      }
      case TC_CREDENTIAL_LEARN_NOT_SUPPORTED: {
        expected_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
        supports_cl = false;
        break;
      }
      case TC_ADD_SLOT_OCCUPIED: {
        cl_status = CL_STATUS_INVALID_ADD_OPERATION_TYPE;
        is_report_expected = true;

        CC_UserCredential_get_credential_ExpectAndReturn(
          uuid, type, slot, NULL, NULL, U3C_DB_OPERATION_RESULT_FAIL_OCCUPIED
          );
        CC_UserCredential_get_credential_IgnoreArg_credential_data();
        CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
        break;
      }
      case TC_MODIFY_DOES_NOT_EXIST: {
        operation_type = U3C_OPERATION_TYPE_MODIFY;
        cl_status = CL_STATUS_INVALID_MODIFY_OPERATION_TYPE;
        is_report_expected = true;

        CC_UserCredential_get_credential_ExpectAndReturn(
          uuid, type, slot, NULL, NULL, U3C_DB_OPERATION_RESULT_FAIL_DNE
          );
        CC_UserCredential_get_credential_IgnoreArg_credential_data();
        CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
        break;
      }
      case TC_CONTROL: {
        expected_status = RECEIVED_FRAME_STATUS_SUCCESS;
        cl_status = CL_STATUS_STARTED;
        steps_remaining = 1;
        is_report_expected = true;

        CC_UserCredential_get_credential_ExpectAndReturn(
          uuid, type, slot, NULL, NULL, U3C_DB_OPERATION_RESULT_FAIL_DNE
          );
        CC_UserCredential_get_credential_IgnoreArg_credential_data();
        CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
        break;
      }
      case TC_ALREADY_IN_PROGRESS: {
        CC_UserCredential_get_credential_ExpectAndReturn(
          uuid, type, slot, NULL, NULL, U3C_DB_OPERATION_RESULT_FAIL_DNE
          );
        cl_status = CL_STATUS_ALREADY_IN_PROGRESS;
        is_report_expected = true;

        CC_UserCredential_get_credential_IgnoreArg_credential_data();
        CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
        break;
      }
      default: {
        break;
      }
    }

    // Set up inputs and outputs of the command handler
    command_handler_input_t input;
    test_common_clear_command_handler_input(&input);
    input.rxOptions.sourceNode.nodeId = 1;
    input.rxOptions.destNode.nodeId = 2;
    ZW_CREDENTIAL_LEARN_START_FRAME incomingFrame;
    u3c_event_data_learn_start_t cl_start_args = {
      .target = {
        .uuid = uuid,
        .type = type,
        .slot = slot,
      },
      .timeout_seconds = timeout,
      .operation_type = operation_type
    };
    credential_learn_start_create(&incomingFrame, cl_start_args);
    input.frame.as_zw_application_tx_buffer.ZW_CredentialLearnStartFrame = incomingFrame;
    ZW_APPLICATION_TX_BUFFER output;
    uint8_t lengthOut = 0;

    uint8_t cl_report_raw[sizeof(ZW_CREDENTIAL_LEARN_REPORT_FRAME)];
    if (is_report_expected) {
      // Set up expected Credential Learn Status Report frame
      memset(cl_report_raw, 0, sizeof(cl_report_raw));
      ZW_CREDENTIAL_LEARN_REPORT_FRAME * cl_report = (ZW_CREDENTIAL_LEARN_REPORT_FRAME *)cl_report_raw;

      cl_report->cmdClass = COMMAND_CLASS_USER_CREDENTIAL;
      cl_report->cmd = CREDENTIAL_LEARN_REPORT;
      cl_report->credentialLearnStatus = cl_status;
      cl_report->userUniqueIdentifier1 = uuid >> 8;
      cl_report->userUniqueIdentifier2 = uuid & 0xFF;
      cl_report->credentialType = type;
      cl_report->credentialSlot1 = slot >> 8;
      cl_report->credentialSlot2 = slot & 0xFF;
      cl_report->credentialLearnStepsRemaining = steps_remaining;

      zaf_transport_tx_ExpectAndReturn(cl_report_raw, sizeof(cl_report_raw), NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_callback();
      zaf_transport_tx_IgnoreArg_zaf_tx_options();
    }

    // Set up relevant mock calls
    if (
      (tc != TC_UUID_ZERO)
      && (tc != TC_CREDENTIAL_SLOT_ZERO)
      && (tc != TC_ALREADY_IN_PROGRESS)
      ) {
      cc_user_credential_get_max_credential_slots_ExpectAndReturn(type, max_slots); cc_user_credential_is_credential_type_supported_ExpectAndReturn(type, supports_type);
      cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(max_uuids);
      cc_user_credential_get_max_credential_slots_ExpectAndReturn(type, max_slots);
      cc_user_credential_is_credential_learn_supported_ExpectAndReturn(type, supports_cl);
    }

    // Process command
    received_frame_status_t status =
      invoke_cc_handler_v2(&input.rxOptions,
                           &input.frame.as_zw_application_tx_buffer,
                           input.frameLength, &output, &lengthOut);

    // Verify outgoing frame status
    TEST_ASSERT_EQUAL_MESSAGE(
      expected_status, status, "The Credential Learn handler returned an invalid status."
      );
  }
}
