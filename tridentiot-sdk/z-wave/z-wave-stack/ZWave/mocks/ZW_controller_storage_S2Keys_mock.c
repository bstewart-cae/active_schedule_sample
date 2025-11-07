// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_controller_storage_S2Keys_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_controller_network_info_storage.h"
#include "mock_control.h"

#define MOCK_FILE "ZW_controller_storage_S2Keys_mock.c"

bool StorageGetS2Keys(Ss2_keys * keys)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, keys);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, keys);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool StorageSetS2Keys(Ss2_keys * keys)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, keys);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, keys);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool StorageGetS2KeyClassesAssigned(Ss2_keyclassesAssigned * assigned)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, assigned);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, assigned);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool StorageSetS2KeyClassesAssigned(Ss2_keyclassesAssigned * assigned)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, assigned);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, assigned);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
