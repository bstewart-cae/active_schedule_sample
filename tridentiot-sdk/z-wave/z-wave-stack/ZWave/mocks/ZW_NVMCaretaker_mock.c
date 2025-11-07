// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_NVMCaretaker_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_NVMCaretaker.h"
#include <string.h>
#include "mock_control.h"

#define MOCK_FILE "ZW_NVMCaretaker.c"

ECaretakerStatus NVMCaretakerVerifySet(const SObjectSet* pObjectSet, ECaretakerStatus * pObjectSetStatus)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ECTKR_STATUS_SUCCESS );
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE);
  MOCK_CALL_ACTUAL(p_mock, pObjectSet, pObjectSetStatus);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT32(p_mock, ARG0, pObjectSet, SObjectSet, pFileSystem);
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT32(p_mock, ARG0, pObjectSet, SObjectSet, iObjectCount);

  MOCK_CALL_COMPARE_STRUCT_ARRAY_UINT32(p_mock, ARG0, pObjectSet, SObjectSet, pObjectDescriptors, iObjectCount * 2 );

  MOCK_CALL_COMPARE_INPUT_POINTER(
            p_mock,
            ARG1,
            pObjectSetStatus);

  if(NULL != p_mock->output_arg[ARG1].p)
  {
    memcpy(pObjectSetStatus, p_mock->output_arg[ARG1].p, sizeof(uint32_t) * pObjectSet->iObjectCount);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, ECaretakerStatus);
}

ECaretakerStatus NVMCaretakerVerifyObject(zpal_nvm_handle_t pFileSystem, const SObjectDescriptor* pObjectDescriptor)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ECTKR_STATUS_SUCCESS );
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE);
  MOCK_CALL_ACTUAL(p_mock, pFileSystem, pObjectDescriptor);

  MOCK_CALL_COMPARE_INPUT_POINTER(
            p_mock,
            ARG0,
            pFileSystem);


  MOCK_CALL_COMPARE_INPUT_POINTER(
            p_mock,
            ARG1,
            pObjectDescriptor);

  MOCK_CALL_RETURN_VALUE(p_mock, ECaretakerStatus);
}

