// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_transmit_cb_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>

#define MOCK_FILE "ZW_transport_transmit_cb_mock.c"
#include <ZW_transport_transmit_cb.h>

bool ZW_TransmitCallbackIsBound(const STransmitCallback* pThis)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, pThis);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pThis);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void ZW_TransmitCallbackBind(STransmitCallback* pThis, ZW_SendData_Callback_t pCallback, ZW_Void_Function_t Context)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pThis, pCallback, Context);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pThis);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pCallback);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, Context);
}

void ZW_TransmitCallbackUnBind(STransmitCallback* pThis)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pThis);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pThis);
}

void ZW_TransmitCallbackInvoke(const STransmitCallback* pThis, uint8_t txStatus, TX_STATUS_TYPE* pextendedTxStatus)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pThis, txStatus, pextendedTxStatus);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pThis);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, txStatus);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pextendedTxStatus);

  if(pThis->pCallback) {
    pThis->pCallback(pThis->Context, 1, pextendedTxStatus);
  }
}
