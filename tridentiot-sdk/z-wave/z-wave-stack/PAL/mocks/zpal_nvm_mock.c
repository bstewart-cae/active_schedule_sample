// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_nvm_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <string.h>
#include <mock_control.h>
#include <zpal_nvm.h>

#define MOCK_FILE "zpal_nvm_mock.c"

zpal_nvm_handle_t zpal_nvm_init(zpal_nvm_area_t area)
{
  mock_t *p_mock;
  static uint8_t handle;

  MOCK_CALL_RETURN_IF_USED_AS_STUB((zpal_nvm_handle_t)&handle);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NULL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_nvm_handle_t);

  MOCK_CALL_ACTUAL(p_mock, area);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, area);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_nvm_handle_t);
}

zpal_status_t zpal_nvm_read_FAKE(zpal_nvm_handle_t handle, uint32_t key, void *object, size_t object_size)
{
  memset(object, 0, object_size);

  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_nvm_read(zpal_nvm_handle_t handle, uint32_t key, void *object, size_t object_size)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(zpal_nvm_read_FAKE, handle, key, object, object_size);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle, key, object, object_size);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, key);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, object);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG3, object_size);

  if(NULL != p_mock->output_arg[ARG2].p)
  {
    MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[2].p, ((uint8_t *)object), object_size, uint8_t);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_read_object_part(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, void *object, size_t offset, size_t object_size)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle, key, object, offset, object_size);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, key);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, object);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG3, offset);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG4, object_size);

  if(NULL != p_mock->output_arg[ARG2].p)
  {
    MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[2].p, ((uint8_t *)object), object_size, uint8_t);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_write(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, const void *object, size_t object_size)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle, key, object, object_size);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, key);
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG2, p_mock->expect_arg[ARG3].v, ((uint8_t *)object), object_size);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_erase_all(zpal_nvm_handle_t handle)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_erase_object(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle, key);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, key);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_get_object_size(zpal_nvm_handle_t handle, zpal_nvm_object_key_t key, size_t *len)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle, key, len);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, key);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, len);

  if (p_mock->output_arg[2].p != NULL)
  {
    MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[2].p, len, 1, size_t);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

size_t zpal_nvm_enum_objects(zpal_nvm_handle_t handle,
                             zpal_nvm_object_key_t *key_list,
                             size_t key_list_size,
                             zpal_nvm_object_key_t key_min,
                             zpal_nvm_object_key_t key_max)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, handle, key_list, key_list_size, key_min, key_max);


  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, key_list);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, key_list_size);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG3, key_min);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG4, key_max);

  if (NULL != p_mock->output_arg[ARG1].p)
  {
    if (p_mock->return_code.v <= key_list_size)
    {
      MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[ARG1].p, key_list, p_mock->return_code.v, zpal_nvm_object_key_t);
    }
    else
    {
      MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[ARG1].p, key_list, key_list_size, zpal_nvm_object_key_t);
    }
  }

  MOCK_CALL_RETURN_VALUE(p_mock, size_t);
}

zpal_status_t zpal_nvm_backup_open(void)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

void zpal_nvm_backup_close(void)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

zpal_status_t zpal_nvm_backup_read(uint32_t offset, void *data, size_t data_length)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, offset, data, data_length);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, offset);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, data);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, data_length);

  MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[1].p, ((uint8_t *)data), data_length, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_backup_write(uint32_t offset, const void *data, size_t data_length)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, offset, data, data_length);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, offset);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, data);

  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG1, p_mock->expect_arg[ARG2].v, ((uint8_t *)data), data_length);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

size_t zpal_nvm_backup_get_size(void)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, size_t);

  MOCK_CALL_RETURN_VALUE(p_mock, size_t);
}

zpal_status_t zpal_nvm_lock(zpal_nvm_handle_t handle)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, handle);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t zpal_nvm_migrate_legacy_app_file_system(void)
{
  return ZPAL_STATUS_OK;
}
