/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "unity.h"
#include "unity_print.h"
#include <string.h>
#include "zpal_init.h"
#include "Assert_mock.h"

void setUpSuite(void)
{
  // not used
}

void tearDownSuite(void)
{
  // not used
}

void setUp(void)
{
  Assert_Ignore();
}

void tearDown(void)
{
  // not used
}

void zpal_enable_watchdog(bool)
{
  // stub function
}
uint32_t Get_RTC_Status(void)
{
  return 0;
}
void Rtc_Disable_Alarm(void)
{
  // stub function
}

void test_zpal_init_set_library_type(void)
{
  zpal_status_t status;
  zpal_library_type_t lib_type;
  lib_type = zpal_get_library_type();
  TEST_ASSERT_EQUAL(ZPAL_LIBRARY_TYPE_UNDEFINED, lib_type);
  status = zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_UNDEFINED);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);
  status = zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_CONTROLLER);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_OK, status);
  status = zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_CONTROLLER);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);
  status = zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_END_DEVICE);
  TEST_ASSERT_EQUAL(ZPAL_STATUS_FAIL, status);
  lib_type = zpal_get_library_type();
  status = zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_CONTROLLER);
}

