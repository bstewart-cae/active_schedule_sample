// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_replication_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include "ZW_transport_api.h"
#include "ZW_controller_api.h"

#define MOCK_FILE "ZW_replication_mock.c"

void ZW_ReplicationReceiveComplete(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}


uint8_t ReplicationSend(uint8_t  destNodeID, uint8_t *pData, uint8_t  dataLength, uint8_t  txOptions, VOID_CALLBACKFUNC(completedFunc)(uint8_t))
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, destNodeID, pData, dataLength, txOptions, completedFunc);

  TEST_FAIL_MESSAGE("Not implemented.");
  return 0x00;
}

void ReplicationSendDoneCallback(uint8_t bStatus)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bStatus);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bStatus);
}

void ReplicationStart(uint8_t rxStatus, uint8_t bSource)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, rxStatus, bSource);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, rxStatus);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bSource);
}

void PresentationReceived(uint8_t rxStatus, uint8_t bSource)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, rxStatus, bSource);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, rxStatus);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bSource);
}


void ZW_NewController(uint8_t bState, VOID_CALLBACKFUNC(completedFunc)(uint8_t, uint8_t, uint8_t*, uint8_t))
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bState, completedFunc);

  TEST_FAIL_MESSAGE("Not implemented.");

}


void TransferDoneCallback(uint8_t bStatus)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, bStatus);
  TEST_FAIL_MESSAGE("Not implemented.");
}


void ZCB_SendTransferEnd(uint8_t state, TX_STATUS_TYPE *txStatusReport)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, state, txStatusReport);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}

void TransferNodeInfoReceived(uint8_t bCmdLength, uint8_t *pCmd)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bCmdLength, pCmd);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}

void TransferRangeInfoReceived(uint8_t bCmdLength, uint8_t *pCmd)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bCmdLength, pCmd);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}


void TransferCmdCompleteReceived(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

uint8_t SendSUCID(uint8_t node, uint8_t txOption, VOID_CALLBACKFUNC(callfunc)(uint8_t, TX_STATUS_TYPE *))
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, txOption, callfunc);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void ReplicationInit(uint8_t bMode, void ( *completedFunc)(LEARN_INFO_T *))
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bMode, completedFunc);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}

void StartReplicationReceiveTimer(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

