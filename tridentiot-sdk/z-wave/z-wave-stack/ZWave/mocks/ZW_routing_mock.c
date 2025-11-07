// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include <NodeMask.h>
#include <string.h>
#include <Assert.h>

#define MOCK_FILE "ZW_routing_mock.c"

void RestartAnalyseRoutingTable(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void UpdateMostUsedNodes(bool bLastTxStatus, uint8_t bEntryPoint)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bLastTxStatus, bEntryPoint);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bLastTxStatus);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bEntryPoint);
}

void ClearMostUsed(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void DeleteMostUsed(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void SetPendingDiscovery(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void ClearPendingDiscovery(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void MergeNoneListeningNodes(uint8_t sourceNode, uint8_t* pRangeFrame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, sourceNode, pRangeFrame);

  TEST_FAIL_MESSAGE("Not implemented.");
}

void MergeNoneListeningSensorNodes(uint8_t sourceNode, uint8_t* pRangeFrame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, sourceNode, pRangeFrame);

  TEST_FAIL_MESSAGE("Not implemented.");
}

void PreferredSet(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void PreferredRemove(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void AnalyseRoutingTable(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void InitRoutingValues(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void RoutingAddNonRepeater(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void RoutingRemoveNonRepeater(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
}

void RoutingAnalysisStop(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void MarkSpeedAsTried(uint8_t speed, uint8_t* pRoutInfo)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, speed, pRoutInfo);

  TEST_FAIL_MESSAGE("Not implemented.");
}

void ClearTriedSpeeds(uint8_t* pRoutInfo)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);  
  MOCK_CALL_ACTUAL(pMock, pRoutInfo);

  TEST_FAIL_MESSAGE("Not implemented.");
}

void SetRoutingLink(uint8_t bANodeID, uint8_t bBNodeID, bool _bset)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bANodeID, bBNodeID, _bset);

  TEST_FAIL_MESSAGE("Not implemented.");
}

bool IsNodeRepeater(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint8_t RoutingInfoReceived(uint8_t bLength, uint8_t bNode, uint8_t *pRangeInfo)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(2); // Return value must be greater than 1 for success.
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, bLength, bNode, pRangeInfo);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bNode);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pRangeInfo);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t RangeInfoNeeded(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

bool ZCB_HasNodeANeighbour(uint8_t bNode)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, bNode);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint8_t FindBestRouteToNode(uint8_t bRepeater, uint8_t bDestination, uint8_t *pHops, uint8_t *pRouting)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bRepeater, bDestination, pHops, pRouting);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t GetNextRouteToNode(
#ifdef ZW_ROUTE_DIVERSITY
  uint8_t bNodeID, uint8_t bCheckMaxAttempts, uint8_t *pRepeaterList, uint8_t *pHopCount, uint8_t *speed)
#else
  uint8_t bNodeID, uint8_t bLastNode, uint8_t *pRepeaterList, uint8_t *pHopCount)
#endif
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
#ifdef ZW_ROUTE_DIVERSITY
  MOCK_CALL_ACTUAL(pMock, bNodeID, bCheckMaxAttempts, pRepeaterList, pHopCount, speed);
#else
  MOCK_CALL_ACTUAL(pMock, bNodeID, bLastNode, pRepeaterList, pHopCount);
#endif  
 
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

bool ZW_SetRoutingInfo(uint8_t bNode, uint8_t bLength, uint8_t* pMask)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, bNode, bLength, pMask);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint8_t GetNextRouteFromNodeToNode(uint8_t bDestNodeID, uint8_t  bSourceNodeID, uint8_t *pRepeaterList, uint8_t *pHopCount, uint8_t bRouteNumber, bool fResetNextNode)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bDestNodeID, bSourceNodeID, pRepeaterList, pHopCount, bRouteNumber, fResetNextNode);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

bool DoesRouteSupportTriedSpeed(uint8_t *pRepeaterList, uint8_t bHopCount, uint8_t bTriedSpeeds)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, pRepeaterList, bHopCount, bTriedSpeeds);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool NextLowerSpeed(uint8_t bSrcNodeID, uint8_t bDestNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bSrcNodeID, bDestNodeID);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

bool DoesRouteSupportHigherSpeed(uint8_t *pRepeaterList, uint8_t bRepeaterCount, uint8_t bRouteSpeed, uint8_t bDestID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, pRepeaterList, bRepeaterCount, bRouteSpeed, bDestID);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool ZW_AreNodesNeighbours(uint8_t bNodeA, uint8_t bNodeB)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, bNodeA, bNodeB);
  
  TEST_FAIL_MESSAGE("Not implemented.");
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void ZW_GetRoutingInfo(uint8_t bNodeID, uint8_t* pMask, uint8_t bOptions)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID, pMask, bOptions);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bNodeID);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pMask);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, bOptions);

  if (NULL != pMask)
  {
    memcpy(pMask, (uint8_t*)pMock->output_arg[1].p, MAX_NODEMASK_LENGTH);
  }
  else
  {
    ASSERT(false); // pMask should never be NULL
  }
}

void ResetRouting(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}
