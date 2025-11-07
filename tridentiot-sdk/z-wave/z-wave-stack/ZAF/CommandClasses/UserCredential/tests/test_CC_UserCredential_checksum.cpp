#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>
#include "test_CC_UserCredential.hpp"

extern "C" {
    #include "ZW_classcmd.h"
    #include "ZAF_CC_Invoker.h"
    #include "unity.h"
    #include "cc_user_credential_config_api_mock.c"
    #include "zaf_transport_tx_mock.h"
    #include "cc_user_credential_io_mock.h"
    #include "CC_Notification_mock.h"
    #include "cc_user_credential_config_api.h"
    #include "cc_user_credential_config.h"
    #include "ZAF_nvm_mock.h"
    #include "CRC.h"
}

static const uint16_t test_user_Matt_uuid = 1;
unsigned char test_user_Matt_name[] = "Matt";
static u3c_user_t test_user_Matt = { 0 };
static u3c_credential_t test_user_Matt_credentials[4];
static uint8_t test_user_Matt_credential_data1[4] = { '9', '2', '7', '7' };
static uint8_t test_user_Matt_credential_data2[6] = { '9', '5', '4', '9', '8', '8' };
static uint8_t test_user_Matt_credential_data3[34] = { '\0', 'z', '\0', 'w', '\0', 'a', '\0', 'v', '\0', 'e', '\0', 'n', '\0', 'o', '\0', 'd', '\0', 'e', '\0', 'p', '\0', 'a', '\0', 's', '\0', 's', '\0', 'w', '\0', 'o', '\0', 'r', '\0', 'd' };
static uint8_t test_user_Matt_credential_data4[2] = { 0x71, 0x99 };

static const uint16_t test_user_Lillie_uuid = 2;
unsigned char test_user_Lillie_name[] = "Lillie";
static u3c_user_t test_user_Lillie = { 0 };
// No credentials for test_user_Lillie

// User Jackie
static const uint16_t test_user_Jackie_uuid = 0x0003;
unsigned char test_user_Jackie_name[] = "Jackie";
static u3c_user_t test_user_Jackie = { 0 };
static u3c_credential_t test_user_Jackie_credentials[4];
static uint8_t test_user_Jackie_credential_data1[4] = { '9', '2', '7', '7' };
static uint8_t test_user_Jackie_credential_data2[6] = { '9', '5', '4', '9', '8', '8' };
static uint8_t test_user_Jackie_credential_data3[34] = { '\0', 'z', '\0', 'w', '\0', 'a', '\0', 'v', '\0', 'e', '\0', 'n', '\0', 'o', '\0', 'd', '\0', 'e', '\0', 'p', '\0', 'a', '\0', 's', '\0', 's', '\0', 'w', '\0', 'o', '\0', 'r', '\0', 'd' };
static uint8_t test_user_Jackie_credential_data4[2] = { 0x45, 0x83 };

// User Mike
static const uint16_t test_user_Mike_uuid = 0x0005;
unsigned char test_user_Mike_name[] = "Mike";
static u3c_user_t test_user_Mike = { 0 };
static u3c_credential_t test_user_Mike_credentials[2];
static uint8_t test_user_Mike_credential_data1[4] = { '4', '8', '1', '2' };
static uint8_t test_user_Mike_credential_data2[2] = { 0x27, 0x55 };

// User with Unique Identifier 0x0007
static const uint16_t test_user_7_uuid = 0x0007;
unsigned char test_user_7_name[] = "";
static u3c_user_t test_user_7 = { 0 };
// No credentials for test_user_7

// Credential Data
u3c_credential_t test_credentials_for_checksum[4];
static uint8_t credential_data1[4] = { '9', '2', '7', '7' };
static uint8_t credential_data2[6] = { '9', '5', '4', '9', '8', '8' };
static uint8_t credential_data3[34] = { '\0', 'z', '\0', 'w', '\0', 'a', '\0', 'v', '\0', 'e', '\0', 'n', '\0', 'o', '\0', 'd', '\0', 'e', '\0', 'p', '\0', 'a', '\0', 's', '\0', 's', '\0', 'w', '\0', 'o', '\0', 'r', '\0', 'd' };
static uint8_t credential_data4[2] = { 0x24, 0x01 };

static void init_test_user_a(void);
static void init_test_user_b(void);
static void init_test_user_jackie(void);
static void init_test_user_mike(void);
static void init_test_user_7(void);
static void init_test_credentials_checksum(void);

void setUpSuite(void)
{
  // All users checksum test participants
  init_test_user_a();
  init_test_user_b();

  // User checksum test participants
  init_test_user_jackie();
  init_test_user_mike();
  init_test_user_7();

  // Credentials checksum test credentials
  init_test_credentials_checksum();
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

using namespace std;

//******************************************************************************
// All Users Checksum test cases
//******************************************************************************

/**
 * @brief This test verifies that if the all users checksum is not supported the
 * all users checksum get will be ignored.
 * CC:0083.01.14.11.000
 */
void test_ALL_USERS_CHECKSUM_GET_ignore(void)
{
  vector<uint8_t> frame = CCUserCredential::AllUsersChecksumGet();
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test All Users Checksum must be unsupported.
  cc_user_credential_is_all_users_checksum_supported_ExpectAndReturn(false);

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT, status, "Failed to process command");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.14.11.001
 * CC:0083.01.15.11.007
 * CC:0083.01.15.11.008
 * CC:0083.01.15.11.009
 */
void test_ALL_USERS_CHECKSUM_GET_validate_example1(void)
{
  vector<uint8_t> frame = CCUserCredential::AllUsersChecksumGet();
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test All Users Checksum must be supported.
  cc_user_credential_is_all_users_checksum_supported_ExpectAndReturn(true);

  CC_UserCredential_get_next_user_ExpectAndReturn(0, test_user_Jackie_uuid);

  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_ReturnArrayThruPtr_name(test_user_Jackie_name, sizeof(test_user_Jackie_name));
  CC_UserCredential_get_user_ReturnThruPtr_user(&test_user_Jackie);

  // There are four credentials for Jackie
  uint8_t current_credential_slot = 0;
  u3c_credential_type current_credential_type = CREDENTIAL_TYPE_NONE;

  for (int i = 0; i < 4; i++) {
    // Mocking CC_UserCredential_get_next_credential
    CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Jackie_uuid, current_credential_type, current_credential_slot, NULL, NULL, true);
    CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
    CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
    CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_user_Jackie_credentials[i].metadata.slot);
    CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_user_Jackie_credentials[i].metadata.type);

    // Update current credential slot and type
    current_credential_type = test_user_Jackie_credentials[i].metadata.type;
    current_credential_slot = test_user_Jackie_credentials[i].metadata.slot;

    // Mocking CC_UserCredential_get_credential
    CC_UserCredential_get_credential_ExpectAndReturn(test_user_Jackie_uuid, current_credential_type, current_credential_slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
    CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
    CC_UserCredential_get_credential_IgnoreArg_credential_data();
    CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_user_Jackie_credentials[i].metadata);
    CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_user_Jackie_credentials[i].data, test_user_Jackie_credentials[i].metadata.length);
  }

  // No more credentials for the test_user_Jackie
  CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Jackie_uuid, current_credential_type, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  CC_UserCredential_get_next_user_ExpectAndReturn(test_user_Jackie_uuid, test_user_Mike_uuid);

  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_ReturnArrayThruPtr_name(test_user_Mike_name, sizeof(test_user_Mike_name));
  CC_UserCredential_get_user_ReturnThruPtr_user(&test_user_Mike);

  // There are two credentials for Mike
  current_credential_slot = 0;
  current_credential_type = CREDENTIAL_TYPE_NONE;

  for (int i = 0; i < 2; i++) {
    // Mocking CC_UserCredential_get_next_credential
    CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Mike_uuid, current_credential_type, current_credential_slot, NULL, NULL, true);
    CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
    CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
    CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_user_Mike_credentials[i].metadata.slot);
    CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_user_Mike_credentials[i].metadata.type);

    // Update current credential slot and type
    current_credential_slot = test_user_Mike_credentials[i].metadata.slot;
    current_credential_type = test_user_Mike_credentials[i].metadata.type;

    // Mocking CC_UserCredential_get_credential
    CC_UserCredential_get_credential_ExpectAndReturn(test_user_Mike_uuid, current_credential_type, current_credential_slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
    CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
    CC_UserCredential_get_credential_IgnoreArg_credential_data();
    CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_user_Mike_credentials[i].metadata);
    CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_user_Mike_credentials[i].data, test_user_Mike_credentials[i].metadata.length);
  }

  // No more credentials for the test_user_Mike
  CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Mike_uuid, current_credential_type, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  CC_UserCredential_get_next_user_ExpectAndReturn(test_user_Mike_uuid, test_user_7_uuid);

  // There are no credentials for User 7
  current_credential_slot = 0;
  current_credential_type = CREDENTIAL_TYPE_NONE;

  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_ReturnArrayThruPtr_name(test_user_7_name, sizeof(test_user_7_name));
  CC_UserCredential_get_user_ReturnThruPtr_user(&test_user_7);

  // No credentials for User 7

  // No more credentials for User 7
  CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_7_uuid, current_credential_type, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  CC_UserCredential_get_next_user_ExpectAndReturn(test_user_7_uuid, 0);

  CCUserCredential::AllUsersChecksumReport ExpectedReport = CCUserCredential::AllUsersChecksumReport();
  ExpectedReport.checksum(0xE602);   // CC:0083.01.15.11.009
  vector<uint8_t> expected_report = ExpectedReport;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

/**
 * @brief This test case verifies that the all users checksum get will return a NVM error during
 * get user operation.
 */
void test_ALL_USERS_CHECKSUM_GET_nvm_error(void)
{
  vector<uint8_t> frame = CCUserCredential::AllUsersChecksumGet();
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test All Users Checksum must be supported.
  cc_user_credential_is_all_users_checksum_supported_ExpectAndReturn(true);

  CC_UserCredential_get_next_user_ExpectAndReturn(0, test_user_Jackie_uuid);

  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_ERROR_IO);

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Status was success despite database error");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.15.11.010
 */
void test_ALL_USERS_CHECKSUM_validate_example2(void)
{
  vector<uint8_t> frame = CCUserCredential::AllUsersChecksumGet();
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test All Users Checksum must be supported.
  cc_user_credential_is_all_users_checksum_supported_ExpectAndReturn(true);

  CC_UserCredential_get_next_user_ExpectAndReturn(0, 0);

  CCUserCredential::AllUsersChecksumReport ExpectedReport = CCUserCredential::AllUsersChecksumReport();
  ExpectedReport.checksum(0x0000);   // CC:0083.01.15.11.010
  vector<uint8_t> expected_report = ExpectedReport;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

//******************************************************************************
// User Checksum test cases
//******************************************************************************

/**
 * @brief This test case verifies that the User Checksum Get will be ignored when this functionality
 * is disabled in the `cc_user_credential_config.h`
 * CC:0083.01.16.11.000
 */
void test_USER_CHECKSUM_GET_ignore(void)
{
  uint16_t example_uuid = 0xABCD;
  vector<uint8_t> frame = CCUserCredential::UserChecksumGet().uuid(example_uuid);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be unsupported.
  cc_user_credential_is_user_checksum_supported_ExpectAndReturn(false);

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT, status, "Failed to process command");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.16.11.001
 * CC:0083.01.17.11.007
 * CC:0083.01.17.11.008
 * CC:0083.01.17.11.009
 */
void test_USER_CHECKSUM_GET_validate_example1(void)
{
  vector<uint8_t> frame = CCUserCredential::UserChecksumGet().uuid(test_user_Matt_uuid);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_user_checksum_supported_ExpectAndReturn(true);

  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_ReturnArrayThruPtr_name(test_user_Matt_name, sizeof(test_user_Matt_name));
  CC_UserCredential_get_user_ReturnThruPtr_user(&test_user_Matt);

  // There are four credentials for the test_user_Matt
  uint8_t current_credential_slot = 0;
  u3c_credential_type current_credential_type = CREDENTIAL_TYPE_NONE;

  for (int i = 0; i < 4; i++) {
    // Mocking CC_UserCredential_get_next_credential
    CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Matt_uuid, current_credential_type, current_credential_slot, NULL, NULL, true);
    CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
    CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
    CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_user_Matt_credentials[i].metadata.slot);
    CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_user_Matt_credentials[i].metadata.type);

    // Update current credential slot and type
    current_credential_slot = test_user_Matt_credentials[i].metadata.slot;
    current_credential_type = test_user_Matt_credentials[i].metadata.type;

    // Mocking CC_UserCredential_get_credential
    CC_UserCredential_get_credential_ExpectAndReturn(test_user_Matt_uuid, current_credential_type, current_credential_slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
    CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
    CC_UserCredential_get_credential_IgnoreArg_credential_data();
    CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_user_Matt_credentials[i].metadata);
    CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_user_Matt_credentials[i].data, test_user_Matt_credentials[i].metadata.length);
  }

  // No more credentials for the test_user_Matt
  CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Matt_uuid, current_credential_type, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::UserChecksumReport ExpectedReport = CCUserCredential::UserChecksumReport();
  ExpectedReport.uuid(test_user_Matt_uuid);
  ExpectedReport.checksum(0x9024);   // CC:0083.01.17.11.009
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

/**
 * @brief This test case verifies that the User Checksum Get will return a NVM error during
 * get user operation.
 */
void test_USER_CHECKSUM_GET_validate_nvm_error(void)
{
  vector<uint8_t> frame = CCUserCredential::UserChecksumGet().uuid(test_user_Matt_uuid);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_user_checksum_supported_ExpectAndReturn(true);

  // Return NVM error in this test
  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_ERROR_IO);

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Status was success despite database error");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.16.11.001
 * CC:0083.01.17.11.010
 * CC:0083.01.17.11.011
 * CC:0083.01.17.11.012
 */
void test_USER_CHECKSUM_GET_validate_example2(void)
{
  vector<uint8_t> frame = CCUserCredential::UserChecksumGet().uuid(test_user_Lillie_uuid);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_user_checksum_supported_ExpectAndReturn(true);

  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_user_ReturnArrayThruPtr_name(test_user_Lillie_name, sizeof(test_user_Lillie_name));
  CC_UserCredential_get_user_ReturnThruPtr_user(&test_user_Lillie);

  // There are no credentials for the test_user_Lillie
  uint8_t current_credential_slot = 0;
  u3c_credential_type current_credential_type = CREDENTIAL_TYPE_NONE;

  // No more credentials for the test_user_Lillie
  CC_UserCredential_get_next_credential_ExpectAndReturn(test_user_Lillie_uuid, current_credential_type, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::UserChecksumReport ExpectedReport = CCUserCredential::UserChecksumReport();
  ExpectedReport.uuid(test_user_Lillie_uuid);
  ExpectedReport.checksum(0xF900);   // CC:0083.01.17.11.012
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.16.11.001
 * CC:0083.01.17.11.013
 */
void test_USER_CHECKSUM_GET_validate_example3(void)
{
  uint16_t example_uuid3 = 0x0010;   // CC:0083.01.17.11.013
  vector<uint8_t> frame = CCUserCredential::UserChecksumGet().uuid(example_uuid3);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_user_checksum_supported_ExpectAndReturn(true);

  // No test user exists with the given UUID
  CC_UserCredential_get_user_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_FAIL_DNE);

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::UserChecksumReport ExpectedReport = CCUserCredential::UserChecksumReport();
  ExpectedReport.uuid(example_uuid3);
  ExpectedReport.checksum(0x0000);   // CC:0083.01.17.11.013
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

//******************************************************************************
// Credential Checksum test cases
//******************************************************************************

/**
 * @brief This test case verifies that the Credential Checksum Get will be ignored when this functionality
 *        is disabled in the `cc_user_credential_config.h`
 */
void test_CREDENTIAL_CHECKSUM_ignored(void)
{
  uint8_t credential_type = 0xAB;
  vector<uint8_t> frame = CCUserCredential::CredentialChecksumGet().credential_type(credential_type);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be unsupported.
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(false);

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT, status, "Failed to process command");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.19.11.001
 * CC:0083.01.19.11.007
 * CC:0083.01.19.11.008
 * CC:0083.01.19.11.009
 */
void test_CREDENTIAL_CHECKSUM_validate_example1(void)
{
  u3c_credential_type example_cred = CREDENTIAL_TYPE_PIN_CODE;
  vector<uint8_t> frame = CCUserCredential::CredentialChecksumGet().credential_type(example_cred);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(true);

  uint8_t current_credential_slot = 0;

  // If uuid 0 then it will search for the credential in the database
  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, true);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_credentials_for_checksum[0].metadata.slot);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_credentials_for_checksum[0].metadata.type);

  CC_UserCredential_get_credential_ExpectAndReturn(0, test_credentials_for_checksum[0].metadata.type, test_credentials_for_checksum[0].metadata.slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
  CC_UserCredential_get_credential_IgnoreArg_credential_data();
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_credentials_for_checksum[0].metadata);
  CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_credentials_for_checksum[0].data, test_credentials_for_checksum[0].metadata.length);

  current_credential_slot = test_credentials_for_checksum[0].metadata.slot;

  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, true);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_credentials_for_checksum[1].metadata.slot);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_credentials_for_checksum[1].metadata.type);

  CC_UserCredential_get_credential_ExpectAndReturn(0, test_credentials_for_checksum[1].metadata.type, test_credentials_for_checksum[1].metadata.slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
  CC_UserCredential_get_credential_IgnoreArg_credential_data();
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_credentials_for_checksum[1].metadata);
  CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_credentials_for_checksum[1].data, test_credentials_for_checksum[1].metadata.length);

  current_credential_slot = test_credentials_for_checksum[1].metadata.slot;

  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::CredentialChecksumReport ExpectedReport = CCUserCredential::CredentialChecksumReport();
  ExpectedReport.credential_type(example_cred);
  ExpectedReport.checksum(0xD867);
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

/**
 * @brief This test case verifies that the Credential Checksum Get will return a NVM error during
 *        get credential operation.
 */
void test_CREDENTIAL_CHECKSUM_nvm_error(void)
{
  u3c_credential_type example_cred = CREDENTIAL_TYPE_PIN_CODE;
  vector<uint8_t> frame = CCUserCredential::CredentialChecksumGet().credential_type(example_cred);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(true);

  // If uuid 0 then it will search for the credential in the database
  // DUMMY TARGETS
  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, 0, NULL, NULL, true);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_credentials_for_checksum[0].metadata.slot);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_credentials_for_checksum[0].metadata.type);

  // NVM error during the call
  CC_UserCredential_get_credential_ExpectAnyArgsAndReturn(U3C_DB_OPERATION_RESULT_ERROR_IO);

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Status was success despite database error");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.19.11.001
 * CC:0083.01.19.11.010
 * CC:0083.01.19.11.011
 * CC:0083.01.19.11.012
 */
void test_CREDENTIAL_CHECKSUM_validate_example2(void)
{
  u3c_credential_type example_cred = CREDENTIAL_TYPE_PASSWORD;
  vector<uint8_t> frame = CCUserCredential::CredentialChecksumGet().credential_type(example_cred);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(true);

  uint8_t current_credential_slot = 0;

  // If uuid 0 then it will search for the credential in the database
  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, true);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_credentials_for_checksum[2].metadata.slot);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_credentials_for_checksum[2].metadata.type);

  CC_UserCredential_get_credential_ExpectAndReturn(0, test_credentials_for_checksum[2].metadata.type, test_credentials_for_checksum[2].metadata.slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
  CC_UserCredential_get_credential_IgnoreArg_credential_data();
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_credentials_for_checksum[2].metadata);
  CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_credentials_for_checksum[2].data, test_credentials_for_checksum[2].metadata.length);

  current_credential_slot = test_credentials_for_checksum[2].metadata.slot;

  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::CredentialChecksumReport ExpectedReport = CCUserCredential::CredentialChecksumReport();
  ExpectedReport.credential_type(example_cred);
  ExpectedReport.checksum(0x6F76);
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.19.11.001
 * CC:0083.01.19.11.013
 * CC:0083.01.19.11.014
 * CC:0083.01.19.11.015
 */
void test_CREDENTIAL_CHECKSUM_validate_example3(void)
{
  u3c_credential_type example_cred = CREDENTIAL_TYPE_EYE_BIOMETRIC;
  vector<uint8_t> frame = CCUserCredential::CredentialChecksumGet().credential_type(example_cred);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(true);

  uint8_t current_credential_slot = 0;

  // If uuid 0 then it will search for the credential in the database
  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, true);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_slot(&test_credentials_for_checksum[3].metadata.slot);
  CC_UserCredential_get_next_credential_ReturnThruPtr_next_credential_type(&test_credentials_for_checksum[3].metadata.type);

  CC_UserCredential_get_credential_ExpectAndReturn(0, test_credentials_for_checksum[3].metadata.type, test_credentials_for_checksum[3].metadata.slot, NULL, NULL, U3C_DB_OPERATION_RESULT_SUCCESS);
  CC_UserCredential_get_credential_IgnoreArg_credential_metadata();
  CC_UserCredential_get_credential_IgnoreArg_credential_data();
  CC_UserCredential_get_credential_ReturnThruPtr_credential_metadata(&test_credentials_for_checksum[3].metadata);
  CC_UserCredential_get_credential_ReturnArrayThruPtr_credential_data(test_credentials_for_checksum[3].data, test_credentials_for_checksum[3].metadata.length);

  current_credential_slot = test_credentials_for_checksum[3].metadata.slot;

  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::CredentialChecksumReport ExpectedReport = CCUserCredential::CredentialChecksumReport();
  ExpectedReport.credential_type(example_cred);
  ExpectedReport.checksum(0xC06E);
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

/**
 * @brief This test case was made from the example in the specification.
 * This test will use the following CC number.
 * CC:0083.01.19.11.001
 * CC:0083.01.19.11.016
 */
void test_CREDENTIAL_CHECKSUM_validate_example4(void)
{
  u3c_credential_type example_cred = CREDENTIAL_TYPE_RFID_CODE;
  vector<uint8_t> frame = CCUserCredential::CredentialChecksumGet().credential_type(example_cred);
  cout << "Generate  frame length: " << frame.size() << endl;
  cout << "Generate  frame: ";

  for (auto i: frame) {
    printf("%02X ", i);
  }
  cout << endl;

  RECEIVE_OPTIONS_TYPE_EX rxo = { 0 };

  cc_handler_input_t input = {
    .rx_options = &rxo,
    .frame = (ZW_APPLICATION_TX_BUFFER *)&frame[0],
    .length = (uint8_t)frame.size()
  };

  // In this test User Checksum must be supported.
  cc_user_credential_is_credential_checksum_supported_ExpectAndReturn(true);

  uint8_t current_credential_slot = 0;

  CC_UserCredential_get_next_credential_ExpectAndReturn(0, example_cred, current_credential_slot, NULL, NULL, false);
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_type();
  CC_UserCredential_get_next_credential_IgnoreArg_next_credential_slot();

  // Testing expected report with the checksum
  // Creating the expected report
  CCUserCredential::CredentialChecksumReport ExpectedReport = CCUserCredential::CredentialChecksumReport();
  ExpectedReport.credential_type(example_cred);
  ExpectedReport.checksum(0x0000);
  vector<uint8_t> expected_report = ExpectedReport;

  cout << "Expected frame length: " << expected_report.size() << endl;
  cout << "Expected frame: ";
  for (auto i: expected_report) {
    printf("%02X ", i);
  }
  cout << endl;

  // CC_UserCredential_UsersChecksumReport_tx
  zaf_transport_rx_to_tx_options_Expect(&rxo, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();
  zaf_transport_tx_ExpectWithArrayAndReturn(&expected_report[0], expected_report.size(), expected_report.size(), NULL, NULL, 0, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Process command
  received_frame_status_t status;
  status = invoke_cc_handler(&input, NULL);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Failed to process command");
}

static void init_test_user_a(void)
{
  test_user_Matt.active = true;
  test_user_Matt.unique_identifier = test_user_Matt_uuid;
  test_user_Matt.modifier_node_id = 0;
  test_user_Matt.expiring_timeout_minutes = 0;
  test_user_Matt.name_length = sizeof(test_user_Matt_name) - 1;
  test_user_Matt.type = USER_TYPE_NON_ACCESS;
  test_user_Matt.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_Matt.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_Matt.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  test_user_Matt_credentials[0] = {
    .metadata = {
      .uuid = 1,
      .slot = 2,
      .length = 4,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = test_user_Matt_credential_data1,
  };
  test_user_Matt_credentials[1] = {
    .metadata = {
      .uuid = 1,
      .slot = 4,
      .length = 6,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = test_user_Matt_credential_data2,
  };
  test_user_Matt_credentials[2] = {
    .metadata = {
      .uuid = 1,
      .slot = 1,
      .length = sizeof(test_user_Matt_credential_data3),
      .type = CREDENTIAL_TYPE_PASSWORD,
    },
    .data = test_user_Matt_credential_data3,
  };
  test_user_Matt_credentials[3] = {
    .metadata = {
      .uuid = 1,
      .slot = 0x0008,
      .length = 2,
      .type = CREDENTIAL_TYPE_HAND_BIOMETRIC,
    },
    .data = test_user_Matt_credential_data4,
  };
}

static void init_test_user_b(void)
{
  test_user_Lillie.active = true;
  test_user_Lillie.unique_identifier = test_user_Lillie_uuid;
  test_user_Lillie.modifier_node_id = 0;
  test_user_Lillie.expiring_timeout_minutes = 0;
  test_user_Lillie.name_length = sizeof(test_user_Lillie_name) - 1;
  test_user_Lillie.type = USER_TYPE_NON_ACCESS;
  test_user_Lillie.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_Lillie.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_Lillie.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;
}

static void init_test_user_jackie(void)
{
// User Jackie
  test_user_Jackie.active = true;
  test_user_Jackie.unique_identifier = test_user_Jackie_uuid;
  test_user_Jackie.modifier_node_id = 0;
  test_user_Jackie.expiring_timeout_minutes = 0;
  test_user_Jackie.name_length = sizeof(test_user_Jackie_name) - 1;
  test_user_Jackie.type = USER_TYPE_NON_ACCESS;
  test_user_Jackie.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_Jackie.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_Jackie.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  test_user_Jackie_credentials[0] = {
    .metadata = {
      .uuid = test_user_Jackie_uuid,
      .slot = 0x0002,
      .length = 4,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = test_user_Jackie_credential_data1,
  };
  test_user_Jackie_credentials[1] = {
    .metadata = {
      .uuid = test_user_Jackie_uuid,
      .slot = 0x0004,
      .length = 6,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = test_user_Jackie_credential_data2,
  };
  test_user_Jackie_credentials[2] = {
    .metadata = {
      .uuid = test_user_Jackie_uuid,
      .slot = 0x0001,
      .length = sizeof(test_user_Jackie_credential_data3),
      .type = CREDENTIAL_TYPE_PASSWORD,
    },
    .data = test_user_Jackie_credential_data3,
  };
  test_user_Jackie_credentials[3] = {
    .metadata = {
      .uuid = test_user_Jackie_uuid,
      .slot = 0x0008,
      .length = 2,
      .type = CREDENTIAL_TYPE_FINGER_BIOMETRIC,
    },
    .data = test_user_Jackie_credential_data4,
  };
}

static void init_test_user_mike(void)
{
  // User Mike
  test_user_Mike.active = true;
  test_user_Mike.unique_identifier = test_user_Mike_uuid;
  test_user_Mike.modifier_node_id = 0;
  test_user_Mike.expiring_timeout_minutes = 0;
  test_user_Mike.name_length = sizeof(test_user_Mike_name) - 1;
  test_user_Mike.type = USER_TYPE_GENERAL;
  test_user_Mike.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_Mike.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_Mike.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  test_user_Mike_credentials[0] = {
    .metadata = {
      .uuid = test_user_Mike_uuid,
      .slot = 0x0003,
      .length = 4,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = test_user_Mike_credential_data1,
  };
  test_user_Mike_credentials[1] = {
    .metadata = {
      .uuid = test_user_Mike_uuid,
      .slot = 0x0003,
      .length = 2,
      .type = CREDENTIAL_TYPE_FACE_BIOMETRIC,
    },
    .data = test_user_Mike_credential_data2,
  };
}

static void init_test_user_7(void)
{
  // User with Unique Identifier 0x0007
  test_user_7.active = true;
  test_user_7.unique_identifier = test_user_7_uuid;
  test_user_7.modifier_node_id = 0;
  test_user_7.expiring_timeout_minutes = 0;
  test_user_7.name_length = sizeof(test_user_7_name) - 1;
  test_user_7.type = USER_TYPE_DISPOSABLE;
  test_user_7.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_7.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_7.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;
}

static void init_test_credentials_checksum(void)
{
  test_credentials_for_checksum[0] = {
    .metadata = {
      .uuid = 1,
      .slot = 0x0002,
      .length = 4,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = credential_data1,
  };

  test_credentials_for_checksum[1] = {
    .metadata = {
      .uuid = 1,
      .slot = 0x0004,
      .length = 6,
      .type = CREDENTIAL_TYPE_PIN_CODE,
    },
    .data = credential_data2,
  };
  test_credentials_for_checksum[2] = {
    .metadata = {
      .uuid = 1,
      .slot = 0x0001,
      .length = sizeof(credential_data3),
      .type = CREDENTIAL_TYPE_PASSWORD,
    },
    .data = credential_data3,
  };
  test_credentials_for_checksum[3] = {
    .metadata = {
      .uuid = 1,
      .slot = 0x0008,
      .length = 2,
      .type = CREDENTIAL_TYPE_EYE_BIOMETRIC,
    },
    .data = credential_data4,
  };
}
