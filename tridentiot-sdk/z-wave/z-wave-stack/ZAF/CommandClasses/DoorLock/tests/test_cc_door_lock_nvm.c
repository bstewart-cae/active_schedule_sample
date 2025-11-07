/**
 * @file
 *
 * Contains the tests for the Door Lock Command Class NVM module
 */
#include <cc_door_lock_io.h>
#include <ZAF_nvm_mock.h>
#include <ZAF_nvm_app_mock.h>
#include <cc_door_lock_config_api_mock.h>
#include <ZAF_file_ids.h>

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

void test_cc_door_lock_migrate(void)
{
  const cc_door_lock_operation_type_t operation_type = DOOR_OPERATION_CONST;
  const uint8_t inside_door_handle_mode = DOOR_HANDLE_1;
  const uint8_t outside_door_handle_mode = 0;

  static cc_door_lock_data_t application_data;

  application_data.type = operation_type;
  application_data.insideDoorHandleMode = inside_door_handle_mode;
  application_data.outsideDoorHandleMode = outside_door_handle_mode;

  ZAF_nvm_app_read_ExpectAndReturn(0x0000, &application_data, sizeof(cc_door_lock_data_t), ZPAL_STATUS_OK);
  ZAF_nvm_app_read_IgnoreArg_object(); // Ignore because we use it as output
  ZAF_nvm_app_read_ReturnMemThruPtr_object(&application_data, sizeof(cc_door_lock_data_t));

  cc_door_lock_get_operation_type_ExpectAndReturn(operation_type);
  cc_door_lock_get_supported_inside_handles_ExpectAndReturn(inside_door_handle_mode);
  cc_door_lock_get_supported_outside_handles_ExpectAndReturn(outside_door_handle_mode);

  ZAF_nvm_write_ExpectAndReturn(ZAF_FILE_ID_CC_DOOR_LOCK, &application_data, sizeof(cc_door_lock_data_t), ZPAL_STATUS_OK);

  ZAF_nvm_app_erase_object_ExpectAndReturn(0x0000, ZPAL_STATUS_OK);

  cc_door_lock_migrate();
}
