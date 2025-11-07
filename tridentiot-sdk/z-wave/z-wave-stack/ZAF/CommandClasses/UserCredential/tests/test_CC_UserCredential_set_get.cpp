/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2024 Z-Wave Alliance
 */
#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>
#include <initializer_list>
#include "test_CC_UserCredential.hpp"
extern "C" {
    #include "ZW_classcmd.h"
    #include "ZAF_CC_Invoker.h"
    #include <unity.h>
    #include <string.h>
    #include "cc_user_credential_config_api_mock.h"
    #include "zaf_transport_tx_mock.h"
    #include "cc_user_credential_io_mock.h"
    #include "cc_user_credential_config.h"
    #include "association_plus_base_mock.h"
    #include "ZAF_TSE_mock.h"
}

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
  CC_UserCredential_get_next_credential_StopIgnore();
  handleAssociationGetnodeList_StopIgnore();
  zaf_transport_rx_to_tx_options_StopIgnore();
  ZAF_TSE_Trigger_StopIgnore();
}

using namespace std;

/**
 * @brief Verifies that zero is not a valid UUID.
 *
 * CC:0083.01.05.11.004
 */
void test_CREDENTIAL_SET_invalid_uuid(void)
{
  std::vector<uint8_t> frame = CCUserCredential::CredentialSet().uuid(0);

  std::cout << "Generated frame length: " << frame.size() << std::endl;
  std::cout << "Generated frame:" << std::endl;
  for (auto i: frame) {
    printf("%02x ", i);
  }

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame[0]),
    .length = (uint8_t)frame.size()
  };

  cc_user_credential_is_credential_type_supported_IgnoreAndReturn(true);
  cc_user_credential_get_min_length_of_data_IgnoreAndReturn(CC_USER_CREDENTIAL_MIN_DATA_LENGTH_PIN_CODE);
  cc_user_credential_get_max_length_of_data_IgnoreAndReturn(CC_USER_CREDENTIAL_MAX_DATA_LENGTH_PIN_CODE);

  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);

  cc_user_credential_is_credential_type_supported_StopIgnore();
}

typedef struct {
  uint8_t credential_type;
  received_frame_status_t expected_status;
}
credential_type_test_vector_t;

/**
 * @brief Verifies that the Credential Set frame is ignored for invalid credential types.
 *
 * CC:0083.01.0A.11.000
 */
void test_CREDENTIAL_SET_invalid_credential_type(void)
{
  cc_user_credential_get_max_user_unique_idenfitiers_IgnoreAndReturn(20);
  cc_user_credential_get_min_length_of_data_IgnoreAndReturn(0);
  cc_user_credential_get_max_length_of_data_IgnoreAndReturn(10);
  cc_user_credential_get_max_credential_slots_IgnoreAndReturn(99);
  CC_UserCredential_get_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_IgnoreArg_user();
  CC_UserCredential_get_user_IgnoreArg_name();
  CC_UserCredential_get_next_credential_IgnoreAndReturn(false);
  cc_user_credential_get_admin_code_supported_IgnoreAndReturn(false);
  handleAssociationGetnodeList_IgnoreAndReturn(NODE_LIST_STATUS_NO_MORE_NODES);
  zaf_transport_rx_to_tx_options_Ignore();
  ZAF_TSE_Trigger_IgnoreAndReturn(true);
  const set<uint8_t> valid_credential_types = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  RECEIVE_OPTIONS_TYPE_EX rxo = {
    .sourceNode = { .nodeId = 1 },
    .destNode = { .nodeId = 2 }
  };
  received_frame_status_t status;

  for (size_t i = 0; i <= UINT8_MAX; i++) {
    std::vector<uint8_t> frame;
    if (i == CREDENTIAL_TYPE_PIN_CODE) {
      // PIN Code must be valid according to the security considerations
      frame = CCUserCredential::CredentialSet()
              .uuid(1)
              .credential_type(static_cast<CCUserCredential::credential_type_t>(i))
              .credential_length(U3C_CREDENTIAL_TYPE_PIN_CODE_MIN_LENGTH_REQUIREMENT)
              .credential_data({ '2', '4', '6', '8' });
    } else {
      frame = CCUserCredential::CredentialSet()
              .uuid(1)
              .credential_type(static_cast<CCUserCredential::credential_type_t>(i));
    }

    cc_handler_input_t input = {
      .rx_options = &rxo,
      .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame[0]),
      .length = (uint8_t)frame.size()
    };

    std::set<uint8_t>::iterator it = valid_credential_types.find(i);
    bool is_credential_type_supported;
    received_frame_status_t expected_status;

    if (it == valid_credential_types.end()) {
      // Expect fail because the credential type wasn't found in the set of valid types.
      is_credential_type_supported = false;
      expected_status = RECEIVED_FRAME_STATUS_FAIL;
    } else {
      // Expect pass because the credential type was found in the set of valid types.
      is_credential_type_supported = true;
      expected_status = RECEIVED_FRAME_STATUS_SUCCESS;
      CC_UserCredential_add_credential_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
      cc_user_credential_get_max_hash_length_ExpectAndReturn((u3c_credential_type)i, 0);
      zaf_transport_tx_ExpectAnyArgsAndReturn(true);
    }

    cc_user_credential_is_credential_type_supported_ExpectAndReturn((u3c_credential_type)i, is_credential_type_supported);
    CC_UserCredential_get_next_user_IgnoreAndReturn(0);
    status = invoke_cc_handler(&input, NULL);     // Don't expect anything written to output.
    TEST_ASSERT_EQUAL(expected_status, status);
  }

  // Stop ignoring functions to avoid ignoring in the next test.
  // Why is this necessary? Shouldn't it be stopped automatically...?
  cc_user_credential_get_min_length_of_data_StopIgnore();
  cc_user_credential_get_max_length_of_data_StopIgnore();
}

/**
 * @brief Verifies that the Credential Set frame is ignored for invalid operation types.
 *
 * CC:0083.01.0A.11.007
 * CC:0083.01.0A.11.008
 * CC:0083.01.0A.11.009
 */
void test_CREDENTIAL_SET_invalid_operation_type(void)
{
  cc_user_credential_is_credential_type_supported_IgnoreAndReturn(true);
  cc_user_credential_get_max_credential_slots_IgnoreAndReturn(99);
  cc_user_credential_get_min_length_of_data_IgnoreAndReturn(0);
  cc_user_credential_get_max_length_of_data_IgnoreAndReturn(10);
  CC_UserCredential_get_next_user_IgnoreAndReturn(0);
  CC_UserCredential_get_user_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_next_credential_IgnoreAndReturn(false);
  cc_user_credential_get_admin_code_supported_IgnoreAndReturn(false);
  handleAssociationGetnodeList_IgnoreAndReturn(NODE_LIST_STATUS_NO_MORE_NODES);
  zaf_transport_rx_to_tx_options_Ignore();
  ZAF_TSE_Trigger_IgnoreAndReturn(true);

  const set<uint8_t> valid_operation_types = { 0, 1, 2 };

  RECEIVE_OPTIONS_TYPE_EX rxo = {
    .sourceNode = { .nodeId = 1 },
    .destNode = { .nodeId = 2 }
  };
  received_frame_status_t status;

  for (size_t i = 0; i <= UINT8_MAX; i++) {
    std::vector<uint8_t> frame = CCUserCredential::CredentialSet()
                                 .operation_type(static_cast<CCUserCredential::operation_type_t>(i))
                                 // PIN Code is the default credential type, and it MUST contain valid credential data
                                 .credential_length(U3C_CREDENTIAL_TYPE_PIN_CODE_MIN_LENGTH_REQUIREMENT)
                                 .credential_data({ '2', '4', '6', '8' });

    cc_handler_input_t input = {
      .rx_options = &rxo,
      .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame[0]),
      .length = (uint8_t)frame.size()
    };

    received_frame_status_t expected_status;

    // Verify that only 2 bits are passed by expecting fail every time those
    // 2 bits equal 3.
    if (3 == (i % 4)) {
      expected_status = RECEIVED_FRAME_STATUS_FAIL;
    } else {
      expected_status = RECEIVED_FRAME_STATUS_SUCCESS;
      switch (i) {
        case U3C_OPERATION_TYPE_ADD:
          CC_UserCredential_add_credential_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
          break;
        case U3C_OPERATION_TYPE_MODIFY:
          CC_UserCredential_modify_credential_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
          break;
        case U3C_OPERATION_TYPE_DELETE:
          CC_UserCredential_delete_credential_IgnoreAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
          break;
      }
      cc_user_credential_get_max_hash_length_ExpectAndReturn(CREDENTIAL_TYPE_PIN_CODE, 0);
      zaf_transport_tx_ExpectAnyArgsAndReturn(true);
    }

    status = invoke_cc_handler(&input, NULL);     // Don't expect anything written to output.
    TEST_ASSERT_EQUAL(expected_status, status);
  }

  // Stop ignoring functions to avoid ignoring in the next test.
  // Why is this necessary? Shouldn't it be stopped automatically...?
  cc_user_credential_is_credential_type_supported_StopIgnore();
  cc_user_credential_get_min_length_of_data_StopIgnore();
  cc_user_credential_get_max_length_of_data_StopIgnore();
}

/**
 * @brief Verifies that the Credential Set frame is ignored for invalid credential data lengths.
 *
 * CC:0083.01.0A.11.014
 * CC:0083.01.0A.11.015
 */
void test_CREDENTIAL_SET_invalid_credential_length(void)
{
  cc_user_credential_is_credential_type_supported_IgnoreAndReturn(true);

  RECEIVE_OPTIONS_TYPE_EX rxo = {
    .sourceNode = { .nodeId = 1 },
    .destNode = { .nodeId = 2 }
  };

  // Verify fail on mismatch between frame length and credential length.
  vector<uint8_t> frame = CCUserCredential::CredentialSet()
                          .credential_length(2)
                          .credential_data({ 1, 2, 3 }); // Intended mismatch between length and data

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame[0]),
    .length = (uint8_t)frame.size()
  };

  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);

  // Verify fail on too low credential length.
  const CCUserCredential::credential_type_t CREDENTIAL_TYPE = CCUserCredential::credential_type_t::PIN_CODE;
  vector<uint8_t> frame2 = CCUserCredential::CredentialSet()
                           .credential_type(CREDENTIAL_TYPE)
                           .credential_data({ 1, 2, 3 });

  cc_handler_input_t input2 = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame2[0]),
    .length = (uint8_t)frame.size()
  };

  cc_user_credential_get_min_length_of_data_ExpectAndReturn(static_cast<u3c_credential_type>(CREDENTIAL_TYPE), 4);
  status = invoke_cc_handler(&input2, NULL);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);

  // Verify fail on too high credential length.
  vector<uint8_t> frame3 = CCUserCredential::CredentialSet()
                           .credential_type(CREDENTIAL_TYPE)
                           .credential_data({ 1, 2, 3 });

  cc_handler_input_t input3 = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame3[0]),
    .length = (uint8_t)frame.size()
  };

  cc_user_credential_get_min_length_of_data_IgnoreAndReturn(4);
  cc_user_credential_get_max_length_of_data_ExpectAndReturn(static_cast<u3c_credential_type>(CREDENTIAL_TYPE), 2);
  status = invoke_cc_handler(&input3, NULL);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);
}

/**
 * @brief Verifies that zero is not a valid UUID.
 *
 * CC:0083.01.05.11.004
 */
void test_CREDENTIAL_GET_invalid_uuid(void)
{
  std::vector<uint8_t> frame = CCUserCredential::CredentialSet()
                               .uuid(0)
                               .credential_data({ '5', '5', '6', '5' })
                               .credential_length(4);

  std::cout << "Generated frame length: " << frame.size() << std::endl;
  std::cout << "Generated frame:" << std::endl;
  for (auto i: frame) {
    printf("%02x ", i);
  }

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame[0]),
    .length = (uint8_t)frame.size()
  };

  cc_user_credential_config_api_mock_Init();
  cc_user_credential_is_credential_type_supported_IgnoreAndReturn(true);
  cc_user_credential_get_min_length_of_data_IgnoreAndReturn(CC_USER_CREDENTIAL_MIN_DATA_LENGTH_PIN_CODE);
  cc_user_credential_get_max_length_of_data_IgnoreAndReturn(CC_USER_CREDENTIAL_MAX_DATA_LENGTH_PIN_CODE);

  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);
}

/**
 * @brief Verifies that a Credential Report is sent as a response to a Credential Get.
 *
 * CC:0083.01.0B.11.000
 */
void test_CREDENTIAL_GET_REPORT(void)
{
  const uint16_t UUID = 1;
  const CCUserCredential::credential_type_t CREDENTIAL_TYPE = CCUserCredential::credential_type_t::PIN_CODE;
  const uint16_t CREDENTIAL_SLOT = 1;

  std::vector<uint8_t> frame = CCUserCredential::CredentialGet()
                               .uuid(UUID)
                               .credential_type(CREDENTIAL_TYPE)
                               .credential_slot(CREDENTIAL_SLOT);

  std::cout << "Generated frame length: " << frame.size() << std::endl;
  std::cout << "Generated frame:" << std::endl;
  for (auto i: frame) {
    printf("%02x ", i);
  }

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame[0]),
    .length = (uint8_t)frame.size()
  };

  const uint8_t EXPECTED_REPORT[] = {
    COMMAND_CLASS_USER_CREDENTIAL,
    CREDENTIAL_REPORT,
    CREDENTIAL_REP_TYPE_RESPONSE_TO_GET,
    (uint8_t)(UUID >> 8),
    (uint8_t)UUID,
    static_cast<uint8_t>(CREDENTIAL_TYPE),
    (uint8_t)(CREDENTIAL_SLOT >> 8),
    (uint8_t)CREDENTIAL_SLOT,
    0x80,     // CRB
    4,     // Length
    1,
    2,
    3,
    4,
    (uint8_t)MODIFIER_TYPE_Z_WAVE,
    0,     // MSB
    1,     // LSB
    1,     // Next credential type must be 1 for now as the XML doesn't contain 0 making
           // the x86 test system unable to parse the frame if set to zero.
    0,
    0,
  };

  u3c_credential_metadata_t metadata = {
    .uuid = UUID,
    .slot = 1,
    .modifier_node_id = 1,
    .length = 4,
    .modifier_type = MODIFIER_TYPE_Z_WAVE,
    .type = CREDENTIAL_TYPE_PIN_CODE,
  };
  uint8_t credential_data[4] = { 3, 4, 9, 4 };
  u3c_credential_type next_credential_type = CREDENTIAL_TYPE_PIN_CODE;
  uint16_t next_credential_slot = 0;

  CC_UserCredential_get_next_credential_ExpectAndReturn(UUID, (u3c_credential_type)CREDENTIAL_TYPE, CREDENTIAL_SLOT, NULL, NULL, true);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&next_credential_type);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&next_credential_slot);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_credential_ExpectAndReturn(UUID, (u3c_credential_type)CREDENTIAL_TYPE, CREDENTIAL_SLOT, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&metadata);
  CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(credential_data, sizeof(credential_data));
  CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
  CC_UserCredential_get_credential_IgnoreArg_credential_data();
  CC_UserCredential_get_next_credential_IgnoreAndReturn(false);
  cc_user_credential_get_max_hash_length_ExpectAndReturn((u3c_credential_type)CREDENTIAL_TYPE, 0);
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(EXPECTED_REPORT, 1, sizeof(EXPECTED_REPORT), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_SUCCESS, status);
}

static void clear_buffer(ZW_APPLICATION_TX_BUFFER const * const p_buffer)
{
  memset((uint8_t *)p_buffer, 0, sizeof(ZW_APPLICATION_TX_BUFFER));
}

/**
 * @brief Verifies that the CC will respond with a USER_CREDENTIAL_ASSOCIATION_REPORT frame
 *        indicating that the source UUID and/or the destination UUID is invalid.
 *
 * Verifies the following requirements:
 * CC:0083.01.12.11.003
 */
void test_USER_CREDENTIAL_ASSOCIATION_SET(void)
{
  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  received_frame_status_t status = RECEIVED_FRAME_STATUS_CC_NOT_FOUND;

  const uint16_t INVALID_DESTINATION_UUID = 0;

  ZW_APPLICATION_TX_BUFFER buffer = { 0 };

  cc_handler_output_t output = {
    .frame = &buffer
  };

  // Verify for destination UUID = 0
  clear_buffer(&buffer);
  std::vector<uint8_t> frame2 = CCUserCredential::UserCredentialAssociationSet()
                                .destination_uuid(INVALID_DESTINATION_UUID);

  cc_handler_input_t input2 = {
    .rx_options = &rxo,
    .frame = reinterpret_cast<ZW_APPLICATION_TX_BUFFER *>(&frame2[0]),
    .length = (uint8_t)frame2.size()
  };

  const uint8_t EXPECTED_REPORT2[] = {
    COMMAND_CLASS_USER_CREDENTIAL,
    USER_CREDENTIAL_ASSOCIATION_REPORT,
    CREDENTIAL_TYPE_PIN_CODE,
    0,     // Source Slot (MSB)
    1,     // Source Slot (LSB)
    INVALID_DESTINATION_UUID >> 8,     // Destination UUID (MSB)
    INVALID_DESTINATION_UUID & 0xFF,     // Destination UUID (LSB)
    0,     // Destination Slot (MSB)
    1,     // Destination Slot (LSB)
    0x01     // Status: Source User Unique Identifier Invalid
  };

  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS);
  cc_user_credential_get_max_credential_slots_ExpectAndReturn(CREDENTIAL_TYPE_PIN_CODE, CC_USER_CREDENTIAL_MAX_CREDENTIAL_SLOTS_PIN_CODE);
  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS);
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(EXPECTED_REPORT2, 1, sizeof(EXPECTED_REPORT2), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  status = invoke_cc_handler(&input2, &output);   // Don't expect anything written to output.
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);
}
