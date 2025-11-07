// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_bootloader_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <zpal_bootloader.h>

#define MOCK_FILE "zpal_bootloader_mock.c"


void zpal_bootloader_get_info(zpal_bootloader_info_t *info)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, info);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, info);

  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[0].p, info, 1, zpal_bootloader_info_t);
}

zpal_status_t zpal_bootloader_init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

void zpal_bootloader_reboot_and_install(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

zpal_status_t zpal_bootloader_verify_image(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_bootloader_write_data(uint32_t offset, uint8_t *data, uint16_t length)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, offset, data, length);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, offset);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG2, length);
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG1, length, data, length);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

bool zpal_bootloader_is_first_boot(bool *updated_successfully)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);
  MOCK_CALL_ACTUAL(p_mock, updated_successfully);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, updated_successfully);
  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[0].p, updated_successfully, 1, bool);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void zpal_bootloader_reset_page_counters(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}