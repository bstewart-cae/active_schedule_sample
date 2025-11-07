// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_misc_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <zpal_misc.h>

#define MOCK_FILE "zpal_misc_mock.c"


void zpal_reboot_with_info(const zpal_soft_reset_mfid_t manufacturer_id,
                           const zpal_soft_reset_info_t reset_info)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void zpal_shutdown_handler(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

size_t zpal_get_serial_number_length(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(8);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, size_t);

  MOCK_CALL_RETURN_VALUE(p_mock, size_t);
}

void zpal_get_serial_number(uint8_t *serial_number)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, serial_number);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, serial_number);

  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[0].p, serial_number, 8, uint8_t);
}

bool zpal_in_isr(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

uint8_t zpal_get_chip_type(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t zpal_get_chip_revision(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

void zpal_debug_init(zpal_debug_config_t config)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);

  MOCK_CALL_ACTUAL(p_mock, config);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, config);
}

void zpal_debug_output(const uint8_t *data, uint32_t length)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, data, length);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, data);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, length);
}

void zpal_disable_interrupts(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

uint32_t zpal_get_app_version(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint32_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint32_t);
}

uint8_t zpal_get_app_version_major(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t zpal_get_app_version_minor(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t zpal_get_app_version_patch(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

zpal_chip_se_type_t zpal_get_get_secure_element_type(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_chip_se_type_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_chip_se_type_t);
}

/// @sa https://github.com/Z-Wave-Alliance/z-wave-stack/issues/177
uint16_t zpal_get_hardware_manufacturer_id(void)
{
  mock_t * p_mock;

  // MFG_ID_ZWAVE_ALLIANCE = 0x031D
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x031D);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint16_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint16_t);
}

/// @sa https://github.com/Z-Wave-Alliance/z-wave-stack/issues/177
bool zpal_get_manufacturer_specific_chip_info(uint8_t *p_info, uint8_t *p_info_length, uint8_t info_length_max)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, p_info, p_info_length, info_length_max);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, p_info);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, p_info_length);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, info_length_max);

  uint8_t length = *((uint8_t *)p_mock->output_arg[1].p);
  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[0].p, p_info, length, uint8_t);
  *p_info_length = length;

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
