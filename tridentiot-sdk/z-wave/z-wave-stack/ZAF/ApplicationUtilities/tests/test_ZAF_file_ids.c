// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZAF_file_ids.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <ZAF_file_ids.h>

void test_ZAF_file_ids(void)
{
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x00000, ZAF_FILE_ID_APP_VERSION, "");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x00001, ZAF_FILE_ID_ASSOCIATIONINFO, "");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x00002, ZAF_FILE_ID_USERCODE, "");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x00003, ZAF_FILE_ID_BATTERYDATA, "");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x00004, ZAF_FILE_ID_NOTIFICATIONDATA, "");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x00005, ZAF_FILE_ID_WAKEUPCCDATA, "");
}
