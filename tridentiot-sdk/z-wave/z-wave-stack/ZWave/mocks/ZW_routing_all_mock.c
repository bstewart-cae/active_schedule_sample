// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_all_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include "ZW_Frame.h"
#include "ZW_tx_queue.h"

#define MOCK_FILE "ZW_routing_all_mock.c"

#define MAX_ROUTED_RESPONSES 2
#define MAX_REPEATERS        4
uint8_t  responseRouteNodeID[MAX_ROUTED_RESPONSES];        /* nodeID on the originator */
uint8_t  responseRouteSpeedNumHops[MAX_ROUTED_RESPONSES];  /* Number of hops and RF speed */
uint8_t  responseRouteRepeaterList[MAX_ROUTED_RESPONSES][MAX_REPEATERS]; /* List of repeaters */
uint8_t  responseRouteRFOption[MAX_ROUTED_RESPONSES];

void ZW_LockRoute(bool bLockRoute)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bLockRoute);
}

void RouteCachedReset(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

uint8_t GetResponseRouteIndex(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, bNodeID);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t GetResponseRoute(uint8_t bNodeID, TxQueueElement *frame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, bNodeID, frame);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, frame);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t StoreRoute(uint8_t headerFormatType, uint8_t *pData, ZW_BasicFrameOptions_t* pFrameOption)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, headerFormatType, pData, pFrameOption);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, headerFormatType);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pData);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pFrameOption);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t StoreRouteExplore(uint8_t bNodeID, bool bRouteNoneInverted, uint8_t *pRoute)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bRouteNoneInverted, pRoute);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG1, bRouteNoneInverted);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pRoute);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t ResponseRouteFull(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 1);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void
RemoveResponseRoute(uint8_t bNodeID)
{

}

void ResetRoutedResponseRoutes(void)
{

}
