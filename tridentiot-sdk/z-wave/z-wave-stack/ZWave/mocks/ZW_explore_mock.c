// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_explore_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include <ZW_transport.h>
#include <ZW_transport_api.h>

#define MOCK_FILE "ZW_explore_mock.c"


void ExploreMachine(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void NWIStopTimer(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void NWIStartTimer(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void ExplorePurgeQueue(uint8_t sourceID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, sourceID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, sourceID);
}

void ExploreSetNWI(uint8_t mode)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, mode);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, mode);
}

void ZCB_ExploreComplete(ZW_Void_Function_t Context, uint8_t bTxStatus, TX_STATUS_TYPE* TxStatusReport)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, Context, bTxStatus, TxStatusReport);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}

bool ExploreQueueIsFull(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void IsMyExploreFrame(ZW_ReceiveFrame_t *pRxFrame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pRxFrame);
  
  TEST_FAIL_MESSAGE("Not implemented.");
}

uint8_t ExploreQueueFrame(uint8_t bSrcNodeID,
                          uint8_t bDstNodeID,
                          uint8_t *pData,
                          uint8_t bDataLength,
                          uint8_t bExploreFlag,
                          const STransmitCallback* pCompletedFunc)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0xFF); // != 0x00 => Frame was enqueued
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0x00); // 0x00 => Queue full
  MOCK_CALL_ACTUAL(pMock, bSrcNodeID, bDstNodeID, pData, bDataLength, bExploreFlag, pCompletedFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bSrcNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bDstNodeID);
#if 0
  printf("Mock: ");
  for (uint32_t i = 0; i < 46; i++)
  {
    printf("%02x ", *(pData + i));
  }
  printf("\n");
#endif
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(pMock, ARG2, pMock->expect_arg[3].v, pData, bDataLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG3, bDataLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG4, bExploreFlag);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG5, pCompletedFunc);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t ExploreTransmitSetNWIMode(uint8_t bMode, const STransmitCallback* pCompletedFunc)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bMode, pCompletedFunc);

  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void
ExploreSetTimeout(exploreQueueElementStruct* pQueueElement, uint32_t iTimeout)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pQueueElement, iTimeout);

}
