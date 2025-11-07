// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_tx_queue_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_tx_queue.h>
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"

#define MOCK_FILE "ZW_tx_queue_mock.c"

void TxQueueEmptyEvent_Add(struct sTxQueueEmptyEvent *elm, VOID_CALLBACKFUNC(EmptyEventCallback)(void))
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, elm, EmptyEventCallback);

  TEST_FAIL_MESSAGE("Not implemented.");
}

void TxQueueReleaseElement(TxQueueElement *pFreeTxElement)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pFreeTxElement);

  TEST_FAIL_MESSAGE("Not implemented.");
}

void TxQueueInit(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void TxQueueQueueElement(TxQueueElement *pNewTxElement)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pNewTxElement);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pNewTxElement);
}

uint8_t TxQueueIsEmpty(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void TxQueueServiceTransmit(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}

TxQueueElement* TxQueueGetFreeElement(TxQueue_ElementPriority_t bPriority, bool delayedTx)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(NULL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);
  MOCK_CALL_ACTUAL(pMock, bPriority, delayedTx);
  
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bPriority);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG1, delayedTx);
  MOCK_CALL_RETURN_VALUE(pMock, TxQueueElement* );
}

void TxQueueStartTransmissionPause(uint32_t Timeout)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, Timeout);

  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG0, Timeout);
}

bool
TxQueueBeamACKReceived(
  node_id_t source_node,
  node_id_t destination_node)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, source_node, destination_node);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, source_node);
  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG1, destination_node);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint32_t TxQueueGetOptions(const TxQueueElement *pElement)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pElement);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pElement);
  MOCK_CALL_RETURN_VALUE(pMock, uint32_t);
}

void TxQueueInitOptions(TxQueueElement *pElement, uint32_t options)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pElement, options);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pElement);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG1, options);
}

void TxQueueSetOptionFlags(TxQueueElement *pElement, uint32_t options)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pElement, options);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pElement);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG1, options);
}

void TxQueueClearOptionFlags(TxQueueElement *pElement, uint32_t options)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pElement, options);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pElement);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG1, options);
}
