#include "unity.h"
#include "test_common.h"
#include "ZW_classcmd.h"
#include <stdbool.h>
#include <string.h>
#include "cc_user_credential_config_api_mock.h"
#include "ZAF_nvm_mock.h"
#include "cc_user_credential_io.h"
#include "SizeOf.h"
#include "cc_user_credential_config.h"
#include "cc_user_credential_io_config.h"
#include "cc_user_credential_nvm.h"
#include "ZAF_file_ids.h"

/*
   STATIC FUNCTIONS
 */
static void helper_preparing_user_database(void);

/*
   DEFINITIONS
 */
#define SIZE_READ_USER_NAME_BUFFER        10
#define SIZE_USER_DESCRIPTORS             2

/*
   COMMON TEST VARIABLES
   variables which are used for test inputs and mocks
 */
static const uint16_t test_user_A_uuid = 1;
unsigned char test_user_A_name[] = "AdminA";
static u3c_user_t test_user_A;

static const uint16_t test_user_B_uuid = 2;
unsigned char test_user_B_name[] = "AdminB";
static u3c_user_t test_user_B;

static user_descriptor_t user_descriptor[SIZE_USER_DESCRIPTORS];

static u3c_user_t read_user;
static uint8_t read_user_name[SIZE_READ_USER_NAME_BUFFER];

/*
   For the test that handles more users we can keep track of the users
   Stubs are created to simulate the NVM read and write functions' basic functionality
 */

// Keep track of the users in the database for the stubs
static user_descriptor_t users[U3C_BUFFER_SIZE_USER_DESCRIPTORS];
// Keep track of the number of users in the database for the stubs
static uint16_t n_users = UINT16_MAX;

void setUpSuite(void)
{
}

void tearDownSuite(void)
{
}

void setUp(void)
{
  memset(&test_user_A, 0, sizeof(test_user_A));
  memset(&test_user_B, 0, sizeof(test_user_B));

  test_user_A.active = true;
  test_user_A.unique_identifier = test_user_A_uuid;
  test_user_A.modifier_node_id = 0;
  test_user_A.expiring_timeout_minutes = 0;
  test_user_A.name_length = sizeof(test_user_A_name) - 1;
  test_user_A.type = USER_TYPE_PROGRAMMING;
  test_user_A.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_A.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_A.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  test_user_B.active = true;
  test_user_B.unique_identifier = test_user_B_uuid;
  test_user_B.modifier_node_id = 0;
  test_user_B.expiring_timeout_minutes = 0;
  test_user_B.name_length = sizeof(test_user_B_name) - 1;
  test_user_B.type = USER_TYPE_PROGRAMMING;
  test_user_B.modifier_type = MODIFIER_TYPE_LOCALLY;
  test_user_B.credential_rule = CREDENTIAL_RULE_SINGLE;
  test_user_B.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  user_descriptor[0].object_offset     = 0;
  user_descriptor[0].unique_identifier = test_user_A_uuid;

  user_descriptor[1].object_offset     = 1;
  user_descriptor[1].unique_identifier = test_user_B_uuid;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  cc_user_credential_get_max_user_unique_idenfitiers_ExpectAndReturn(CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS);

  CC_UserCredential_factory_reset();

  ZAF_nvm_write_StopIgnore();

  memset(&read_user, 0, sizeof(u3c_user_t));
  memset(read_user_name, 0, sizeof(read_user_name));

  memset(users, 0, sizeof(users));
  n_users = UINT16_MAX;
}

void tearDown(void)
{
}

void test_USER_CREDENTIAL_IO_get_user_empty_database(void)
{
  u3c_db_operation_result return_value;

  return_value = CC_UserCredential_get_user(test_user_A_uuid, &read_user, NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_FAIL_DNE, return_value,
                                  "[Get User] Getting user from an empty database succeeded");
}

void test_USER_CREDENTIAL_IO_get_user_normal(void)
{
  u3c_db_operation_result return_value;

  helper_preparing_user_database();

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(user_descriptor, 1);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(&test_user_A, 1);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(test_user_A_name, sizeof(test_user_A_name));

  return_value = CC_UserCredential_get_user(test_user_A_uuid, &read_user, read_user_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Get User] Getting user from the database failed");
}

void test_USER_CREDENTIAL_IO_get_next_user_empty_database(void)
{
  uint16_t return_value;
  return_value = CC_UserCredential_get_next_user(test_user_A_uuid);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, return_value,
                                  "[Get Next User] Getting next user from an empty database succeeded");
}

void test_USER_CREDENTIAL_IO_get_next_user_normal(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(user_descriptor, 2);

  return_value = CC_UserCredential_get_next_user(test_user_A_uuid);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(test_user_B_uuid, return_value,
                                  "[Get Next User] Getting next user from database failed");
}

void test_USER_CREDENTIAL_IO_modify_user_empty_database(void)
{
  uint16_t return_value;
  u3c_user_t updated_user;

  memcpy(&updated_user, &test_user_A, sizeof(u3c_user_t));
  updated_user.active = false;

  return_value = CC_UserCredential_modify_user(&updated_user, test_user_A_name);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_FAIL_DNE, return_value,
                                  "[Modify User] Modifying user in empty database succeeded");
}

void test_USER_CREDENTIAL_IO_modify_user_normal(void)
{
  uint16_t return_value;
  u3c_user_t updated_user;

  // Dummy credential metadata that is different from the new metadata
  u3c_user_t metadata = { 0 };

  helper_preparing_user_database();

  // Read user descriptors
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(user_descriptor, 2);

  // Read user metadata
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnThruPtr_object(&metadata);

  memcpy(&updated_user, &test_user_A, sizeof(u3c_user_t));
  updated_user.active = false;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);

  return_value = CC_UserCredential_modify_user(&updated_user, test_user_A_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Modify User] Modifying user failed");
}

void test_USER_CREDENTIAL_IO_delete_user_empty_database(void)
{
  uint16_t return_value;

  return_value = CC_UserCredential_delete_user(test_user_A_uuid);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_FAIL_DNE, return_value,
                                  "[Delete User] Deleting user from empty database succeeded");
}

void test_USER_CREDENTIAL_IO_delete_user_normal(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(user_descriptor, 2);

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  return_value = CC_UserCredential_delete_user(test_user_A_uuid);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Delete User] Deleting user failed");
}

/**
 * @brief This test verifes that the users are stored in ascending order
 *        based on their unique identifier. At the beginning the database
 *        is storing two users with unique identifiers 1 and 3. Then a third
 *        user is added with unique identifier 3. The expected order is 1, 2, 3.
 */
void test_USER_CREDENTIAL_IO_check_users_ascending_order_normal_insert(void)
{
  uint16_t return_value;

  // Initializing two users
  // IMPORTANT: with this the inner n_users variable will be set to 2, the users will be what is returned by the nvm read mock
  helper_preparing_user_database();

  // At the beginning the database is storing two users with unique identifiers 1 and 3
  user_descriptor_t users[U3C_BUFFER_SIZE_USER_DESCRIPTORS];
  memset(users, 0, sizeof(users));

  users[0].unique_identifier = 1;
  users[1].unique_identifier = 3;
  // The nvm read will return the two users from the nvm
  ZAF_nvm_read_ExpectAndReturn(0, NULL, 0, ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_key(); // AREA_USER_DESCRIPTORS
  ZAF_nvm_read_IgnoreArg_object_size();
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(users, U3C_BUFFER_SIZE_USER_DESCRIPTORS);

  // The area users and area_user_name are not part of the test case, so this will be ignored
  ZAF_nvm_write_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);

  // The main test part starts here where the new user will be inserted in to the RAM with
  // unique identifier 2. The expected order is 1, 2, 3. This will be written into the NVM

  // Creating the expected users array
  user_descriptor_t users_write[U3C_BUFFER_SIZE_USER_DESCRIPTORS];
  memset(users_write, 0, sizeof(users_write));

  users_write[0].unique_identifier = 1;
  users_write[1].unique_identifier = 2;
  users_write[2].unique_identifier = 3;
  const uint8_t number_of_users = 3; // users_write[0] users_write[1] users_write[2]
  printf("sizeof(user_descriptor_t: %d", sizeof(user_descriptor_t));
  ZAF_nvm_write_ExpectWithArrayAndReturn(0, users_write, number_of_users, number_of_users * sizeof(user_descriptor_t), ZPAL_STATUS_OK); // First parameter is ignored
  ZAF_nvm_write_IgnoreArg_key(); // AREA_USER_DESCRIPTORS
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK); // AREA_USER_NAMES nvm write - don't care for now

  // Adding a third user with uuid 2, this user is already used in other test cases (user B)
  return_value = CC_UserCredential_add_user(&test_user_B, test_user_B_name);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Add Credential] Adding credential failed");
}

void test_USER_CREDENTIAL_IO_check_users_ascending_order_insert_db_full(void)
{
  u3c_user_t user;
  user.active = true;
  user.unique_identifier = 1;
  user.modifier_node_id = 0;
  user.expiring_timeout_minutes = 0;
  unsigned char test_user_name[] = "User";
  user.name_length = sizeof(test_user_name) - 1;
  user.type = USER_TYPE_PROGRAMMING;
  user.modifier_type = MODIFIER_TYPE_LOCALLY;
  user.credential_rule = CREDENTIAL_RULE_SINGLE;
  user.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  //Mocking the NVM read and write functions
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreAndReturn(ZPAL_STATUS_OK);

  //Prepare the database by adding the maximum number of users
  for (uint16_t i = 1; i <= CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS; i++) {
    user.unique_identifier = i;
    u3c_db_operation_result return_value = CC_UserCredential_add_user(&user, test_user_name);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                    "[Add User] Adding user to the database failed");
  }

  //Try to add one more user and check if the return value is U3C_DB_OPERATION_RESULT_FAIL_FULL
  user.unique_identifier = CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS + 1;
  u3c_db_operation_result return_value = CC_UserCredential_add_user(&user, test_user_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_FAIL_FULL, return_value,
                                  "[Add User] Database owerwrite occured");
}

void test_USER_CREDENTIAL_IO_add_credential_normal(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  u3c_credential_t credential;
  uint8_t credential_data[10];

  memset(credential_data, 0xA5, sizeof(credential_data));
  credential.metadata.length = sizeof(credential_data);
  credential.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credential.metadata.uuid   = test_user_A_uuid;
  credential.metadata.slot   = 1;
  credential.data            = credential_data;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  return_value = CC_UserCredential_add_credential(&credential);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Add Credential] Adding credential failed");
}

void test_USER_CREDENTIAL_IO_add_credential_adding_same_credential_multiple_times(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  u3c_credential_t credential;
  uint8_t credential_data[10];

  memset(credential_data, 0xA5, sizeof(credential_data));
  credential.metadata.length = sizeof(credential_data);
  credential.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credential.metadata.uuid   = test_user_A_uuid;
  credential.metadata.slot   = 1;
  credential.data            = credential_data;

  credential_descriptor_t credentials[1];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credential.metadata.uuid;
  credentials[0].credential_slot        = credential.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credential.metadata.type;

  // Dummy credential metadata that is different from the new metadata
  credential_metadata_nvm_t metadata = { 0 };

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credential);

  // Read credential descriptors
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  // Read credential metadata
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnThruPtr_object(&metadata);

  return_value = CC_UserCredential_add_credential(&credential);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_FAIL_OCCUPIED, return_value,
                                  "[Add Credential] Adding the same credential multiple times succeeded");
}

void test_USER_CREDENTIAL_IO_add_credential_modify_credential(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  u3c_credential_t credential;
  uint8_t credential_data[10];

  memset(credential_data, 0xA5, sizeof(credential_data));
  credential.metadata.length = sizeof(credential_data);
  credential.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credential.metadata.uuid   = test_user_A_uuid;
  credential.metadata.slot   = 1;
  credential.data            = credential_data;

  credential_descriptor_t credentials[1];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credential.metadata.uuid;
  credentials[0].credential_slot        = credential.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credential.metadata.type;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credential);

  // Dummy credential metadata that is different from the new metadata
  credential_metadata_nvm_t metadata = { 0 };

  // Read credential descriptors
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  // Read credential metadata
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnThruPtr_object(&metadata);

  uint8_t new_credential_data[10];
  memset(new_credential_data, 0x5A, sizeof(new_credential_data));
  credential.data            = new_credential_data;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  return_value = CC_UserCredential_modify_credential(&credential);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Modify Credential] Modifying credential failed");
}

void test_USER_CREDENTIAL_IO_delete_credential_normal(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  u3c_credential_t credentialA;
  u3c_credential_t credentialB;
  uint8_t credential_data[10];

  memset(credential_data, 0xA5, sizeof(credential_data));
  credentialA.metadata.length = sizeof(credential_data);
  credentialA.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialA.metadata.uuid   = test_user_A_uuid;
  credentialA.metadata.slot   = 1;
  credentialA.data            = credential_data;

  credentialB.metadata.length = sizeof(credential_data);
  credentialB.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialB.metadata.uuid   = test_user_B_uuid;
  credentialB.metadata.slot   = 2;
  credentialB.data            = credential_data;

  credential_descriptor_t credentials[2];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credentialA.metadata.uuid;
  credentials[0].credential_slot        = credentialA.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credentialA.metadata.type;

  credentials[1].user_unique_identifier = credentialB.metadata.uuid;
  credentials[1].credential_slot        = credentialB.metadata.slot;
  credentials[1].object_offset          = 1;
  credentials[1].credential_type        = credentialB.metadata.type;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialA);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  CC_UserCredential_add_credential(&credentialB);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 2);

  return_value = CC_UserCredential_delete_credential(credentialA.metadata.type, credentialA.metadata.slot);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Delete Credential] Deleting credential failed");
}

void test_USER_CREDENTIAL_IO_move_credential_normal(void)
{
  uint16_t return_value;
  u3c_credential_t credentialA;
  uint8_t credential_data[10];

  helper_preparing_user_database();

  memset(credential_data, 0xA5, sizeof(credential_data));
  credentialA.metadata.length = sizeof(credential_data);
  credentialA.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialA.metadata.uuid   = test_user_A_uuid;
  credentialA.metadata.slot   = 1;
  credentialA.data            = credential_data;

  credential_descriptor_t credentials[1];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credentialA.metadata.uuid;
  credentials[0].credential_slot        = credentialA.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credentialA.metadata.type;

  credential_metadata_nvm_t metadata = { 0 };

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialA);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnThruPtr_object(&metadata);

  ZAF_nvm_write_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  return_value = CC_UserCredential_move_credential(credentialA.metadata.type, credentialA.metadata.slot, test_user_B_uuid, 1);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Move Credential] Moving credential failed");
}

void test_USER_CREDENTIAL_IO_get_credential_normal(void)
{
  uint16_t return_value;
  u3c_credential_t credentialA;
  uint8_t credential_data[10];

  helper_preparing_user_database();

  memset(credential_data, 0xA5, sizeof(credential_data));
  credentialA.metadata.length = sizeof(credential_data);
  credentialA.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialA.metadata.uuid   = test_user_A_uuid;
  credentialA.metadata.slot   = 0;
  credentialA.data            = credential_data;

  credential_descriptor_t credentials[1];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credentialA.metadata.uuid;
  credentials[0].credential_slot        = credentialA.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credentialA.metadata.type;

  credential_metadata_nvm_t credential_metadata;
  memset(&credential_metadata, 0, sizeof(credential_metadata));
  credential_metadata.length           = credentialA.metadata.length;
  credential_metadata.modifier_node_id = 0xA5;
  credential_metadata.modifier_type    = MODIFIER_TYPE_LOCALLY;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialA);

  u3c_credential_metadata_t retreived_metadata;
  uint8_t retreived_credential_data[sizeof(credential_data)];

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(&credential_metadata, 1);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credential_data, 10);

  return_value = CC_UserCredential_get_credential(test_user_A_uuid,
                                                  credentialA.metadata.type,
                                                  credentialA.metadata.slot,
                                                  &retreived_metadata,
                                                  retreived_credential_data);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Get Credential] Getting credential failed");
}

void test_USER_CREDENTIAL_IO_get_next_credential_normal(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  u3c_credential_t credentialA;
  u3c_credential_t credentialB;
  uint8_t credential_data[10];

  memset(credential_data, 0xA5, sizeof(credential_data));
  credentialA.metadata.length = sizeof(credential_data);
  credentialA.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialA.metadata.uuid   = test_user_A_uuid;
  credentialA.metadata.slot   = 0;
  credentialA.data            = credential_data;

  credentialB.metadata.length = sizeof(credential_data);
  credentialB.metadata.type   = CREDENTIAL_TYPE_PASSWORD;
  credentialB.metadata.uuid   = test_user_A_uuid;
  credentialB.metadata.slot   = 1;
  credentialB.data            = credential_data;

  credential_descriptor_t credentials[2];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credentialA.metadata.uuid;
  credentials[0].credential_slot        = credentialA.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credentialA.metadata.type;

  credentials[1].user_unique_identifier = credentialB.metadata.uuid;
  credentials[1].credential_slot        = credentialB.metadata.slot;
  credentials[1].object_offset          = 1;
  credentials[1].credential_type        = credentialB.metadata.type;

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 0);

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialA);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialB);

  uint16_t retreived_next_credential_slot;
  u3c_credential_type retreived_next_credential_type;

  return_value = CC_UserCredential_get_next_credential(test_user_A_uuid,
                                                       credentialA.metadata.type,
                                                       credentialA.metadata.slot,
                                                       &retreived_next_credential_type,
                                                       &retreived_next_credential_slot);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(true, return_value,
                                  "[Get Next Credential] Getting next credential failed");
}

void test_USER_CREDENTIAL_IO_get_next_credential_find_first_credential_for_user_normal(void)
{
  uint16_t return_value;

  helper_preparing_user_database();

  u3c_credential_t credentialA;
  u3c_credential_t credentialB;
  uint8_t credential_data[10];

  memset(credential_data, 0xA5, sizeof(credential_data));
  credentialA.metadata.length = sizeof(credential_data);
  credentialA.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialA.metadata.uuid   = test_user_A_uuid;
  credentialA.metadata.slot   = 0;
  credentialA.data            = credential_data;

  credentialB.metadata.length = sizeof(credential_data);
  credentialB.metadata.type   = CREDENTIAL_TYPE_PASSWORD;
  credentialB.metadata.uuid   = test_user_A_uuid;
  credentialB.metadata.slot   = 1;
  credentialB.data            = credential_data;

  credential_descriptor_t credentials[2];
  memset(credentials, 0, sizeof(credentials));
  credentials[0].user_unique_identifier = credentialA.metadata.uuid;
  credentials[0].credential_slot        = credentialA.metadata.slot;
  credentials[0].object_offset          = 0;
  credentials[0].credential_type        = credentialA.metadata.type;

  credentials[1].user_unique_identifier = credentialB.metadata.uuid;
  credentials[1].credential_slot        = credentialB.metadata.slot;
  credentials[1].object_offset          = 1;
  credentials[1].credential_type        = credentialB.metadata.type;

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialA);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 1);

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_credential(&credentialB);

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 2);

  uint16_t retreived_next_credential_slot;
  u3c_credential_type retreived_next_credential_type;

  return_value = CC_UserCredential_get_next_credential(test_user_A_uuid,
                                                       0,
                                                       0,
                                                       &retreived_next_credential_type,
                                                       &retreived_next_credential_slot);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(true, return_value,
                                  "[Get Next Credential] Finding first credential for user failed");
}

zpal_status_t ZAF_nvm_write_Stub_Callback(zpal_nvm_object_key_t key, const void* object, size_t object_size, int cmock_num_calls)
{
  if (key == ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_DESCRIPTOR_TABLE) {
    memcpy(users, object, object_size);
  } else if (key == ZAF_FILE_ID_CC_USER_CREDENTIAL_NUMBER_OF_USERS) {
    n_users = *(uint16_t*)object;
  }
  return ZPAL_STATUS_OK;
}

zpal_status_t ZAF_nvm_read_Stub_Callback(zpal_nvm_object_key_t key, void* object, size_t object_size, int cmock_num_calls)
{
  if (key == ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_DESCRIPTOR_TABLE) {
    memcpy((void*)object, users, object_size);
  } else if (key == ZAF_FILE_ID_CC_USER_CREDENTIAL_NUMBER_OF_USERS) {
    *(uint16_t*)object = n_users;
  } else if (key >= ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE && key <= ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_LAST) {
    uint16_t user_offset = key - ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE;
    // Search for the user with the given nvm base offset
    for (uint8_t i = 0; i < U3C_BUFFER_SIZE_USER_DESCRIPTORS; i++) {
      if (users[i].object_offset == user_offset) {
        memcpy((void*)object, &users[i], object_size);
        break;
      }
    }
  }
  return ZPAL_STATUS_OK;
}

/**
 * @brief This test verifes that the users are stored in ascending order
 *        based on their unique identifier. At the beginning the database
 *        is storing (maxusers - 1) users with unique identifiers 1, 3, 4, 5, ..., 19. Then a 20th
 *        user is added with unique identifier 2. The expected order is 1, 2, 3. 4, ... 19, 20.
 */
void test_USER_CREDENTIAL_IO_check_users_ascending_order_insert_db_full_ordered(void)
{
  u3c_user_t user;
  user.active = true;
  user.unique_identifier = 1;
  user.modifier_node_id = 0;
  user.expiring_timeout_minutes = 0;
  unsigned char test_user_name[] = "User";
  user.name_length = sizeof(test_user_name) - 1;
  user.type = USER_TYPE_PROGRAMMING;
  user.modifier_type = MODIFIER_TYPE_LOCALLY;
  user.credential_rule = CREDENTIAL_RULE_SINGLE;
  user.name_encoding = USER_NAME_ENCODING_STANDARD_ASCII;

  ZAF_nvm_write_Stub(ZAF_nvm_write_Stub_Callback);
  ZAF_nvm_read_Stub(ZAF_nvm_read_Stub_Callback);

  //Create the database by adding the maximum number of users but one
  for (uint16_t i = 1; i < CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS; i++) {
    user.unique_identifier = (i >= 2) ? i + 1 : i; // 1, 3, 4, 5, ..., 19
    u3c_db_operation_result return_value = CC_UserCredential_add_user(&user, test_user_name);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                    "[Add User] Adding user to the database failed");
  }

  // Create the expected users in the expected ID order (1, 2, 3, 4, ... 19, 20)
  user_descriptor_t reference_users[CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS];
  for (uint16_t i = 0; i < CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS; i++) {
    reference_users[i].unique_identifier = i + 1;
    // reference_users[i].object_offset = i;
  }

  // Add the 20th user with ID = 2
  user.unique_identifier = 2;
  u3c_db_operation_result return_value = CC_UserCredential_add_user(&user, test_user_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Add User] Adding user to the database failed");

  // Get all the 20 users with CC_UserCredential_get_user and check if they are in the expected order
  for (uint16_t i = 0; i < CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS; i++) {
    u3c_user_t read_user;
    memset(&read_user, 0, sizeof(u3c_user_t));

    u3c_db_operation_result return_value = CC_UserCredential_get_user(i + 1, &read_user, NULL);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                    "[Get User] Getting user from the database failed");

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(reference_users[i].unique_identifier, read_user.unique_identifier,
                                     "[Get User] User unique identifier is not as expected");

    // TODO: I'm not sure we need this
    // TEST_ASSERT_EQUAL_UINT16_MESSAGE(reference_users[i].object_offset, read_user.object_offset,
    //                                  "[Get User] User object offset is not as expected");
  }
}

void test_USER_CREDENTIAL_IO_check_users_ascending_order_move_db_ordered(void)
{
  uint16_t return_value;
  u3c_credential_t credentialA, credentialB, credentialC;
  uint8_t credential_data[10];

  // FIXME: Somehow the test is failing if the nvm mock is not initialized here
  ZAF_nvm_mock_Init();

  // Increase n_users to 2
  helper_preparing_user_database();

  // Create credential A
  memset(credential_data, 0xA5, sizeof(credential_data));
  credentialA.metadata.length = sizeof(credential_data);
  credentialA.metadata.type   = CREDENTIAL_TYPE_PIN_CODE;
  credentialA.metadata.uuid   = test_user_A_uuid;
  credentialA.metadata.slot   = 1;
  credentialA.data            = credential_data;

  // Create credential B, same as A but with slot 2
  memcpy(&credentialB, &credentialA, sizeof(u3c_credential_t));
  credentialB.metadata.slot   = 2;
  credentialB.metadata.uuid   = test_user_B_uuid;

  // Create credential C, same as A but with slot 4
  memcpy(&credentialC, &credentialA, sizeof(u3c_credential_t));
  credentialC.metadata.slot   = 4;
  credentialC.metadata.uuid   = 3;

  // Create a descriptor array with the credentials
  credential_descriptor_t credentials[3];
  memset(credentials, 0, sizeof(credentials));

  // Metadata for credential B in NVM
  credential_metadata_nvm_t nvm_metadata_credentialB = { 0 };

  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreAndReturn(ZPAL_STATUS_OK);
  // This part just incrementing n_crednetials internal variable
  CC_UserCredential_add_credential(&credentialA);
  CC_UserCredential_add_credential(&credentialB);
  CC_UserCredential_add_credential(&credentialC);

  ZAF_nvm_read_StopIgnore();
  ZAF_nvm_write_StopIgnore();

  // Load the credentials from the "NVM"
  credentials[0].user_unique_identifier = credentialA.metadata.uuid;
  credentials[0].credential_slot        = credentialA.metadata.slot;
  credentials[0].object_offset          = 1;
  credentials[0].credential_type        = credentialA.metadata.type;

  credentials[1].user_unique_identifier = credentialB.metadata.uuid;
  credentials[1].credential_slot        = credentialB.metadata.slot;
  credentials[1].object_offset          = 2;
  credentials[1].credential_type        = credentialB.metadata.type;

  credentials[2].user_unique_identifier = credentialC.metadata.uuid;
  credentials[2].credential_slot        = credentialC.metadata.slot;
  credentials[2].object_offset          = 3;
  credentials[2].credential_type        = credentialC.metadata.type;

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(credentials, 3); // Return the three credentials

  uint16_t expected_slot = 3;
  credential_descriptor_t expected_credentials[3];
  memcpy(&expected_credentials, &credentials, sizeof(credentials));
  expected_credentials[1].user_unique_identifier = credentialA.metadata.uuid;
  expected_credentials[1].credential_slot        = expected_slot;
  expected_credentials[1].object_offset          = 2;
  expected_credentials[1].credential_type        = credentialB.metadata.type;

  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnThruPtr_object(&nvm_metadata_credentialB);

  ZAF_nvm_write_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);

  ZAF_nvm_write_ExpectWithArrayAndReturn(0, &expected_credentials, 3, 3 * sizeof(credential_descriptor_t), ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreArg_key();

  return_value = CC_UserCredential_move_credential(credentialB.metadata.type, credentialB.metadata.slot, test_user_A_uuid, expected_slot);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, return_value,
                                  "[Move Credential] Moving credential failed");
}

#ifndef AC_PKT_SIZE
#define AC_PKT_SIZE 11
#endif
void test_USER_CREDENTIAL_IO_set_admin_code_good(void)
{
  // Set up expected results
  u3c_admin_code_metadata_t expected_result = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_INTERNAL_NONE,
    .code_data = { 0x33, 0x34, 0x39, 0x34 }
  };

  uint8_t admin_code_pkt[AC_PKT_SIZE] = {
    4,
    0x33,
    0x34,
    0x39,
    0x34,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  };

  // Set up mocks
  ZAF_nvm_write_StopIgnore();
  ZAF_nvm_write_ExpectWithArrayAndReturn(0, (uint8_t*)admin_code_pkt,
                                         AC_PKT_SIZE, AC_PKT_SIZE, ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreArg_key();
  // call function and test
  u3c_db_operation_result result = CC_UserCredential_set_admin_code(&expected_result);
  ;
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_result.result, ADMIN_CODE_OPERATION_RESULT_MODIFIED,
                                  "Returned operation result not successful");

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(result, U3C_DB_OPERATION_RESULT_SUCCESS,
                                  "Database operation failure");
}

void test_USER_CREDENTIAL_IO_set_admin_code_bad(void)
{
  // Set up expected results
  u3c_admin_code_metadata_t expected_result = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_INTERNAL_NONE,
    .code_data = { 0x33, 0x34, 0x39, 0x34 }
  };

  uint8_t admin_code_pkt[AC_PKT_SIZE] = {
    4,
    0x33,
    0x34,
    0x39,
    0x34,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  };

  // Set up mocks
  ZAF_nvm_write_StopIgnore();
  ZAF_nvm_write_ExpectWithArrayAndReturn(0, (uint8_t*)admin_code_pkt,
                                         AC_PKT_SIZE, AC_PKT_SIZE, ZPAL_STATUS_FAIL);
  ZAF_nvm_write_IgnoreArg_key();
  // call function and test
  u3c_db_operation_result result = CC_UserCredential_set_admin_code(&expected_result);
  ;
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_result.result, ADMIN_CODE_OPERATION_RESULT_ERROR_NODE,
                                  "Returned operation result successful when it should not be");

  TEST_ASSERT_NOT_EQUAL_UINT8_MESSAGE(result, U3C_DB_OPERATION_RESULT_SUCCESS,
                                      "Database operation succeeded when it should not have");
  ZAF_nvm_write_StopIgnore();
}

void test_USER_CREDENTIAL_IO_get_admin_code_good(void)
{
  // Set up expected results
  u3c_admin_code_metadata_t expected_result = {
    .code_length = 4,
    .result = ADMIN_CODE_OPERATION_RESULT_GET_RESP,
    .code_data = { 0x33, 0x34, 0x39, 0x34 }
  };

  uint8_t admin_code_pkt[AC_PKT_SIZE] = {
    4,
    0x33,
    0x34,
    0x39,
    0x34,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  };

  u3c_admin_code_metadata_t ac_code = { 0 };
  // Set up mocks
  ZAF_nvm_read_StopIgnore();
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(admin_code_pkt, sizeof(admin_code_pkt)); // Return the three credentials
  // call function and test
  u3c_db_operation_result result = CC_UserCredential_get_admin_code_info(&ac_code);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_SUCCESS, result,
                                  "Returned nvm result that is not accurate");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_result.result, ac_code.result,
                                  "Returned operation result that is not accurate");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_result.code_length, ac_code.code_length,
                                  "Returned operation result that is not accurate");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected_result.code_data, ac_code.code_data, expected_result.code_length,
                                        "Admin code is not what it should be");
  ZAF_nvm_read_StopIgnore();
}

void test_USER_CREDENTIAL_IO_get_admin_code_bad(void)
{
  // Set up expected results
  u3c_admin_code_metadata_t expected_result = {
    .result = ADMIN_CODE_OPERATION_RESULT_ERROR_NODE,
  };

  uint8_t admin_code_pkt[AC_PKT_SIZE] = {
    4,
    0x33,
    0x34,
    0x39,
    0x34,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  };

  u3c_admin_code_metadata_t ac_code = { 0 };
  // Set up mocks
  ZAF_nvm_read_StopIgnore();
  ZAF_nvm_read_ExpectAnyArgsAndReturn(ZPAL_STATUS_FAIL);
  ZAF_nvm_read_IgnoreArg_object();
  ZAF_nvm_read_ReturnArrayThruPtr_object(admin_code_pkt, sizeof(admin_code_pkt)); // Return the three credentials
  // call function and test
  u3c_db_operation_result result = CC_UserCredential_get_admin_code_info(&ac_code);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(U3C_DB_OPERATION_RESULT_ERROR, result,
                                  "Returned nvm result that is not accurate");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_result.result, ac_code.result,
                                  "Returned operation result that is not accurate");
  ZAF_nvm_read_StopIgnore();
}

static void helper_preparing_user_database(void)
{
  ZAF_nvm_read_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_user(&test_user_A, test_user_A_name);
  ZAF_nvm_read_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);
  CC_UserCredential_add_user(&test_user_B, test_user_B_name);

  ZAF_nvm_read_StopIgnore();
  ZAF_nvm_write_StopIgnore();
}
