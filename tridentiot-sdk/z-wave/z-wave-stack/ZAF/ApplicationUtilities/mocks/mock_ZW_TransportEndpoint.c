// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file mock_ZW_TransportEndpoint.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include "ZW_typedefs.h"
#include "ZW_TransportEndpoint.h"
#include <ZW_transport_api.h>
#include "mock_control.h"

#define MOCK_FILE "mock_ZW_TransportEndpoint.c"

void ZW_TransportEndpoint_Init(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void
RxToTxOptions(
        RECEIVE_OPTIONS_TYPE_EX *rxopt,
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX** txopt)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, rxopt, txopt);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, rxopt, RECEIVE_OPTIONS_TYPE_EX, rxStatus);
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, rxopt, RECEIVE_OPTIONS_TYPE_EX, securityKey);
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, rxopt, RECEIVE_OPTIONS_TYPE_EX, sourceNode.nodeId);
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, rxopt, RECEIVE_OPTIONS_TYPE_EX, sourceNode.endpoint);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, txopt);

  *txopt = p_mock->output_arg[1].p;
}

EZAF_EnqueueStatus_t
ZAF_Transmit(
  uint8_t *pData,
  size_t   dataLength,
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx,
  ZAF_TX_Callback_t pCallback)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZAF_ENQUEUE_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZAF_ENQUEUE_STATUS_TIMEOUT);
  MOCK_CALL_ACTUAL(p_mock, pData, dataLength, pTxOptionsEx, pCallback);

  // Compares both array length and array.
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(
          p_mock,
          ARG0,
          p_mock->expect_arg[1].v,
          pData,
          dataLength);

  MOCK_CALL_COMPARE_INPUT_UINT32(
          p_mock,
          ARG1,
          dataLength);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG3,
          pCallback);

  MOCK_CALL_RETURN_VALUE(p_mock, EZAF_EnqueueStatus_t);
}

zaf_cc_list_t*
GetEndpointcmdClassList( bool secList, uint8_t endpoint)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(NULL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NULL);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, secList);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, endpoint);

  MOCK_CALL_RETURN_POINTER(p_mock, zaf_cc_list_t *);

}

bool
Check_not_legal_response_job(RECEIVE_OPTIONS_TYPE_EX *rxOpt)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, true);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, rxOpt);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool
is_multicast(RECEIVE_OPTIONS_TYPE_EX *rxOpt)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, true);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, rxOpt);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void
SetFlagSupervisionEncap(bool flag)
{
}

received_frame_status_t
Transport_ApplicationCommandHandlerEx(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(RECEIVED_FRAME_STATUS_SUCCESS);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, RECEIVED_FRAME_STATUS_FAIL);

  MOCK_CALL_ACTUAL(pMock, rxOpt, pCmd, cmdLength);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, rxOpt);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, cmdLength);
  MOCK_CALL_RETURN_VALUE(pMock, received_frame_status_t);
}
