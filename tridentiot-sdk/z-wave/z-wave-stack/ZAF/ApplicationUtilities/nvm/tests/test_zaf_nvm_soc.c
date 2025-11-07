/**
 * @file
 *
 * @copyright 2022 Silicon Laboratories Inc.
 *
 */
#include <zaf_nvm_soc.h>
#include <ZAF_Common_helper_mock.h>
#include <zpal_misc_mock.h>
#include <ZAF_nvm_mock.h>
#include <ZAF_nvm_app_mock.h>
#include <Assert_mock.h>
#include <ZAF_file_ids.h> // for ZAF_FILE_ID_APP_VERSION, ZAF_FILE_SIZE_APP_VERSION

static bool assert_cb_called;

static void
Assert_cb(__attribute__((unused)) const char* pFileName,
          __attribute__((unused)) int iLineNumber,
          __attribute__((unused)) int cmock_num_calls)
{
  assert_cb_called = true;
}

void
setUpSuite(void)
{
}

void
tearDownSuite(void)
{
}

void
setUp(void)
{
  assert_cb_called = false;
}

void
tearDown(void)
{
}

/**
 * Test zafi_nvm_app_set_default_configuration
 */
void test_set_default_configuration_1(void)
{
  uint32_t current_version;

  current_version = 0x00000001;

  ZAF_Reset_Expect();

  zpal_get_app_version_ExpectAndReturn(current_version);

  ZAF_nvm_write_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &current_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_OK);

  zafi_nvm_app_set_default_configuration();

  TEST_ASSERT_FALSE(assert_cb_called);
}

/**
 * Test zafi_nvm_app_set_default_configuration
 */
void test_set_default_configuration_2(void)
{
  uint32_t current_version;

  current_version = 0x00000001;

  ZAF_Reset_Expect();

  zpal_get_app_version_ExpectAndReturn(current_version);

  Assert_StubWithCallback(Assert_cb);

  ZAF_nvm_write_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &current_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_FAIL);

  zafi_nvm_app_set_default_configuration();

  TEST_ASSERT_TRUE(assert_cb_called);
}

/**
 * Test zafi_nvm_app_reset
 */
void test_reset(void)
{
  uint32_t current_version;

  current_version = 0x00000001;

  ZAF_nvm_app_erase_ExpectAndReturn(true);
  ZAF_nvm_erase_ExpectAndReturn(true);

  ZAF_Reset_Expect();

  zpal_get_app_version_ExpectAndReturn(current_version);

  ZAF_nvm_write_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &current_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_OK);

  zafi_nvm_app_reset();

  TEST_ASSERT_FALSE(assert_cb_called);
}

/**
 * Test zafi_nvm_app_load_configuration
 * ZPAL_STATUS_OK && saved_version == current_version
 */
void test_load_configuration_1(void)
{
  uint32_t saved_version, current_version;

  saved_version = 0x00000001;
  current_version = 0x00000001;

  ZAF_nvm_read_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &saved_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object(); // Used as output
  ZAF_nvm_read_ReturnMemThruPtr_object(&saved_version, sizeof(saved_version));

  zpal_get_app_version_ExpectAndReturn(current_version);

  zafi_nvm_app_load_configuration();

  TEST_ASSERT_FALSE(assert_cb_called);
}

/**
 * Test zafi_nvm_app_load_configuration
 * ZPAL_STATUS_OK && saved_version < current_version
 */
void test_load_configuration_2(void)
{
  uint32_t saved_version, current_version;

  saved_version = 0x00000001;
  current_version = 0x00000002;

  ZAF_nvm_read_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &saved_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_OK);
  ZAF_nvm_read_IgnoreArg_object(); // Used as output
  ZAF_nvm_read_ReturnMemThruPtr_object(&saved_version, sizeof(saved_version));

  zpal_get_app_version_ExpectAndReturn(current_version);

  zafi_nvm_app_load_configuration();

  TEST_ASSERT_FALSE(assert_cb_called);
}

/**
 * Test zafi_nvm_app_load_configuration
 * ZPAL_STATUS_FAIL && saved_version < current_version
 */
void test_load_configuration_3(void)
{
  uint32_t saved_version, current_version;

  saved_version = 0x00000001;
  current_version = 0x00000002;

  ZAF_nvm_read_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &saved_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_FAIL);
  ZAF_nvm_read_IgnoreArg_object(); // Used as output
  ZAF_nvm_read_ReturnMemThruPtr_object(&saved_version, sizeof(saved_version));

  ZAF_nvm_app_erase_ExpectAndReturn(true);
  ZAF_nvm_erase_ExpectAndReturn(true);

  ZAF_Reset_Expect();

  zpal_get_app_version_ExpectAndReturn(current_version);

  ZAF_nvm_write_ExpectAndReturn(ZAF_FILE_ID_APP_VERSION, &current_version, ZAF_FILE_SIZE_APP_VERSION, ZPAL_STATUS_OK);

  zafi_nvm_app_load_configuration();

  TEST_ASSERT_FALSE(assert_cb_called);
}
