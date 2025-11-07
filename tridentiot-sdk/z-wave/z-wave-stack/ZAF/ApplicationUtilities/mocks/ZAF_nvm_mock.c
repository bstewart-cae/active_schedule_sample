/*
 * ZAF_nvm_app_mock.c
 *
 *  Created on: 19/09/2018
 *      Author: JoRoseva
 */

#include "ZAF_nvm.h"
#include <string.h>
#include "mock_control.h"

#define MOCK_FILE "ZAF_nvm_mock.c"

bool ZAF_nvm_init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool ZAF_nvm_erase(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

zpal_status_t ZAF_nvm_erase_object(zpal_nvm_object_key_t key)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_ACTUAL(p_mock, key);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, key);
  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t ZAF_nvm_read(zpal_nvm_object_key_t key, void *object, size_t object_size)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_ACTUAL(p_mock, key, object, object_size);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, key);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, object);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, object_size);

  if(NULL != p_mock->output_arg[ARG1].p)
  {
    MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[1].p, ((uint8_t *)object), object_size, uint8_t);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t ZAF_nvm_read_object_part(zpal_nvm_object_key_t key, void *object, size_t offset, size_t size)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_ACTUAL(p_mock, key, object, size);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, key);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, object);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, offset);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG3, size);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t ZAF_nvm_write(zpal_nvm_object_key_t key, const void *object, size_t object_size)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, key, object, object_size);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, key);
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG1, p_mock->expect_arg[ARG2].v, ((uint8_t *)object), object_size);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t ZAF_nvm_get_object_size(zpal_nvm_object_key_t key, size_t * object_size)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_ACTUAL(p_mock, key, object_size);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, key);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, object_size);

  if (p_mock->output_arg[1].p != NULL)
  {
    MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[1].p, object_size, 1, size_t);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

void zafi_nvm_app_set_default_configuration(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void
zafi_nvm_app_reset(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void zafi_nvm_app_load_configuration(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}