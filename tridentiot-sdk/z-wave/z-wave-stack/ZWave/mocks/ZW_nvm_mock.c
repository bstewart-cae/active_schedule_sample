// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* Mock for ZW_nvm.
*
* @copyright 2018 Silicon Laboratories Inc.
*/

#include "mock_control.h"
#include "unity.h"

#include "ZW_nvm.h"

#define MOCK_FILE "ZW_nvm_mock.c"

ENvmFsInitStatus NvmFileSystemInit(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ENVMFSINITSTATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, ENVMFSINITSTATUS_FAILED);
  MOCK_CALL_RETURN_VALUE(pMock, ENvmFsInitStatus);
}

bool NvmFileSystemFormat(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

zpal_nvm_handle_t NvmFileSystemRegister(const SSyncEvent* pFsResetCb)
{
  mock_t * pMock;
  
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);
  
  MOCK_CALL_ACTUAL(pMock, pFsResetCb);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pFsResetCb);
  if(COMPARE_STRICT == pMock->compare_rule_arg[0])
  {
    MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT32(pMock, ARG0, pFsResetCb, SSyncEvent, pObject);
    MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT32(pMock, ARG0, pFsResetCb, SSyncEvent, uFunctor.pFunction);
  }

  MOCK_CALL_RETURN_POINTER(pMock, zpal_nvm_handle_t);
}

void NVM3_InvokeCbs(void)
{

}
