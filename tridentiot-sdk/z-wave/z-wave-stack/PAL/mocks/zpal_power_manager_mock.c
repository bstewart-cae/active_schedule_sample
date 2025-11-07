// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_power_manager_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <zpal_power_manager.h>
#include "mock_control.h"

#define MOCK_FILE "zpal_power_manager_mock.c"


zpal_pm_handle_t zpal_pm_register(zpal_pm_type_t type)
{
  mock_t * pMock;
  uint8_t _handle = 0;
  zpal_pm_handle_t handle = (zpal_pm_handle_t)&_handle;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(handle);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);

  MOCK_CALL_ACTUAL(pMock, type);

  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG0, type);
  MOCK_CALL_RETURN_POINTER(pMock, zpal_pm_handle_t);
}

void zpal_pm_stay_awake(zpal_pm_handle_t handle, uint32_t msec)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, handle, msec);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG1, msec);
}

void zpal_pm_cancel(zpal_pm_handle_t handle)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, handle);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, handle);
}

void zpal_pm_cancel_all(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}
