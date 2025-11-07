// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_TSE_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZAF_TSE.h>
#include "mock_control.h"

#define MOCK_FILE "ZAF_TSE_mock.c"

bool ZAF_TSE_Init(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock,NULL);

  MOCK_CALL_RETURN_IF_ERROR_SET(pMock, bool);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool ZAF_TSE_Trigger(zaf_tse_callback_t pCallback,
                     void* pData,
                     bool overwrite_previous_trigger)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, pCallback, pData);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pCallback);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pData);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG2, overwrite_previous_trigger);

  MOCK_CALL_RETURN_IF_ERROR_SET(pMock, bool);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void ZAF_TSE_TXCallback(transmission_result_t * pTransmissionResult)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pTransmissionResult);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pTransmissionResult);
}
