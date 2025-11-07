// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_cache_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_protocol_def.h"
#include "ZW_routing_cache.h"
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include "ZW_transport_api.h"
#include <ZW_lib_defines.h>

#define MOCK_FILE "ZW_routing_cache_mock.c"

#define MAX_ROUTED_RESPONSES 2
#define MAX_REPEATERS        4
uint8_t  responseRouteNodeID[MAX_ROUTED_RESPONSES];        /* nodeID on the originator */
uint8_t  responseRouteSpeedNumHops[MAX_ROUTED_RESPONSES];  /* Number of hops and RF speed */
uint8_t  responseRouteRepeaterList[MAX_ROUTED_RESPONSES][MAX_REPEATERS]; /* List of repeaters */
uint8_t  responseRouteRFOption[MAX_ROUTED_RESPONSES];

void LastWorkingRouteCacheLineExploreUpdate(uint8_t bNodeID, bool bRouteNoneInverted, uint8_t *pRoute)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bRouteNoneInverted, pRoute);
}

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


uint8_t LastWorkingRouteCacheInit(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t LastWorkingRouteCacheCurrentLWRSet(uint8_t bNodeID, uint8_t bValue)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bValue);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t LastWorkingRouteCacheCurrentLWRGet(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t LastWorkingRouteCacheCurrentLWRToggle(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t LastWorkingRouteCacheNodeSRLocked(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void LastWorkingRouteCacheNodeSRLockedSet(node_id_t bNodeID, uint8_t bValue)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bValue);
}

uint8_t LastWorkingRouteCacheLineExists(uint8_t bNodeID, uint8_t bLWRno)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bLWRno);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

S_LWR_STORE_RES LastWorkingRouteCacheLineStore(uint8_t bNodeID, uint8_t bLWRno)
{
  mock_t * pMock;
  S_LWR_STORE_RES retvalStub = {0, };
  S_LWR_STORE_RES retvalFailure = {1, };

  MOCK_CALL_RETURN_IF_USED_AS_STUB(retvalStub);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, retvalFailure);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  
  return *((S_LWR_STORE_RES*)(pMock->return_code.p));
}

#ifdef MULTIPLE_LWR
void LastWorkingRouteCacheLineUpdate(uint8_t bNodeID, uint8_t bLWRno, uint8_t *pData)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bLWRno, pData);
}

uint8_t LastWorkingRouteCacheLineGet(uint8_t bNodeID, uint8_t bLWRno, TxQueueElement *frame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bLWRno, frame);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t LastWorkingRouteCacheLineExile(uint8_t bNodeID, uint8_t bLWRno)                  
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID, bLWRno);
  
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}
#else
void LastWorkingRouteCacheLinePurge(uint8_t bNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
}

void LastWorkingRouteCacheLineUpdate(uint8_t bNodeID, uint8_t *pData)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bNodeID, pData);
}

bool LastWorkingRouteCacheLineGet(uint8_t bNodeID, TxQueueElement *frame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, bNodeID, frame);
  
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
#endif
